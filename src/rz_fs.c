#include "rz_fs.h"
#include "rz_error.h"
#include "rz_sprintf.h"

#define RZ_TAG "rz_fs"

#ifdef RZ_OS_WINDOWS
/* --- Windows path predicates --- */
#    define rz__path_is_drive_letter(path)          (path.len < 2) && ((rz_in_range(path.data[0], 'A', 'Z') || rz_in_range(path.data[0], 'a', 'z')) || (path.data[1] == ':'))
#    define rz__path_is_drive_absolute_strict(path) ((path.len >= 3) && rz__path_is_drive_letter(path) && rz__path_is_sep(path.data[2]))
#    define rz__path_is_unc(path)                   rz_sv_starts_with(path, rz_sv_static("\\\\"))
#    define rz__path_is_verbatim(path)              (rz_sv_starts_with(path, rz_sv_static("\\\\?\\")) || rz_sv_starts_with(path, rz_sv_static("\\??\\")))
#    define rz__path_is_device_namespace(path)      rz_sv_starts_with(path, rz_sv_static("\\\\.\\"))
#    define RZ__FILE_SHARE_MODE_DEFAULT             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE
#endif
#define rz__path_is_rooted_current_drive(path) (path.len > 0) && rz__path_is_sep(path.data[0])

RZ_DEF const char *rz_pathbuf_cstr(RZ_PathBuf *path) {
    RZ_DBG_ASSERT_NOT_NULL(path);
    if (path->len < path->capacity) {
        if (*rz_arr_end(path) == '\0') {
            return path->data;
        } else {
            *rz_arr_end(path) = '\0';
            return path->data;
        }
    } else {
        rz_str_append_null(path);
        return path->data;
    }
}

RZ_DEF bool rz_pathbuf_set_filename(RZ_PathBuf *path, RZ_Path filename) {
    RZ_DBG_ASSERT_NOT_NULL(path);
    if (rz_arr_is_empty(path) || *path->data == '\0') return false;

    RZ_StrView prev_filename = {0};
    if (rz_path_filename(rz_path_from_pathbuf(path), &prev_filename)) {
        // truncate or remove the filename part from path.
        auto begin_prev_filename = rz_arr_begin(&prev_filename);
        auto begin_orig          = path->data;
        rz_arr_truncate(path, begin_prev_filename - begin_orig);
    }
    rz_pathbuf_push(path, filename);
    return true;
}

RZ_DEF bool rz_pathbuf_set_filestem(RZ_PathBuf *path, RZ_Path filestem) {
    RZ_Path filename = {0};
    if (rz_path_filename(rz_path_from_pathbuf(path), &filename)) {
        // truncate or remove the filename part from path.
        auto begin_filename = rz_arr_begin(&filename);
        auto begin_orig     = path->data;
        rz_arr_truncate(path, begin_filename - begin_orig);
    }

    RZ_TEMP_ALLOCATOR_BLOCK(a, {
        RZ_Path extension = {0};
        if (!rz_arr_is_empty(&filename)) {
            // if path contains filename. add the extension to the path again.
            // find the last dot.
            rz_usize dot = rz_sv_rfind_char(filename, '.');
            if (dot != RZ_NOT_FOUND) {
                RZ_StrView t = rz_sv_strip_nsuffix(&filename, dot);
                // to prevent overwrite of extension. duplicate using temp allocator.
                extension = (RZ_StrView){
                    .data = rz_memdup(a, t.data, t.len),
                    .len  = t.len,
                };
            }
        }

        // push the filestem to the path
        rz_pathbuf_push(path, filestem);
        if (!rz_arr_is_empty(&extension)) {
            rz_str_append_sv(path, extension);
            rz_str_append_null(path);
        }
    });

    return true;
}

#define rz__path_validate_extension(ext) RZ_ASSERT(rz_sv_contains_char(ext, RZ_PATH_SEP), "extension cannot contain path separators: `" RZ_SVFmt "`", RZ_SVArg(ext))

RZ_DEF bool rz_pathbuf_set_extension(RZ_PathBuf *path, RZ_Path extension) {
    rz__path_validate_extension(extension);

    RZ_StrView filestem = {0};
    // if does not have file stem. its mean the last element of the path is '/' or '..'.
    // its mean, the path is not formed to set an extension. so return false
    if (rz_arr_is_empty(path) || !rz_path_filestem(rz_path_from_pathbuf(path), &filestem)) return false;

    // truncate until right after the file stem
    auto end_file_stem = rz_arr_end(&filestem);
    auto start         = path->data;
    rz_arr_truncate(path, end_file_stem - start);

    // add the new extension, if any.
    if (!rz_arr_is_empty(&extension)) {
        if (rz_arr_first(&extension) != '.') rz_str_append(path, '.');
        rz_str_append_sv(path, extension);
    }

    rz_str_append_null(path);
    return true;
}

RZ_DEF bool rz_pathbuf_add_extension(RZ_PathBuf *path, RZ_Path extension) {
    if (!rz_arr_is_empty(&extension)) {
        rz__path_validate_extension(extension);

        // truncate until right after the file name
        // this is necessary for trimming the trailing separator
        RZ_StrView filename = {0};
        if (rz_arr_is_empty(path) || !rz_path_filename(rz_path_from_pathbuf(path), &filename)) return false;
        auto end_filename = rz_arr_end(&filename);
        auto start        = path->data;
        rz_arr_truncate(path, end_filename - start);

        if (rz_arr_first(&extension) != '.') rz_str_append(path, '.');
        rz_str_append_sv(path, extension);

        rz_str_append_null(path);
    }

    return true;
}

#define rz__path_is_sep(expr) (((expr) == RZ_PATH_WINDOWS_SEP) || ((expr) == RZ_PATH_UNIX_SEP))

RZ_DEF RZ_PathBuf rz_pathbuf_join(const RZ_PathBuf *path, RZ_Path component) {
    RZ_PathBuf res = rz_pathbuf_clone(path);
    rz_pathbuf_push(&res, component);
    return res;
}

static void rz__path_change_sep(RZ_Path *path) {
    rz_foreach(ch, path) {
#if defined(RZ_OS_WINDOWS)
        if (*ch == RZ_PATH_UNIX_SEP) *ch = RZ_PATH_SEP;
#else
        if (*ch == RZ_PATH_WINDOWS_SEP) *ch = RZ_PATH_SEP;
#endif
    }
}

RZ_DEF void rz_pathbuf_push(RZ_PathBuf *path, RZ_Path component) {
    RZ_DBG_ASSERT(path != NULL && !rz_arr_is_empty(&component));

    rz__path_change_sep(&component);
    if (!rz_arr_is_empty(path)) {

        auto last     = rz_arr_last(path);
        bool need_sep = !rz__path_is_sep(last);

        if (rz_path_is_absolute(component)) {
            rz_arr_truncate(path, 0);
        } else if (need_sep) {
            rz_str_append(path, RZ_PATH_SEP);
        }
    }

    rz_str_append_sv(path, component);
    rz_str_append_null(path);
    return;
}

RZ_DEF bool rz_pathbuf_canonicalize(RZ_PathBuf *path) {
    const char *cstr_path = rz_pathbuf_cstr(path);
    char       *buf       = NULL;
    rz_usize    buf_len   = RZ_PATH_MAX;

    auto a                = rz_temp_allocator();
    buf                   = rz_alloc_bytes(a, buf_len);

#if defined(RZ_OS_WINDOWS)
    bool result = true;
    auto save   = rz_temp_snapshot();

    HANDLE h    = CreateFileA(cstr_path, 0, RZ__FILE_SHARE_MODE_DEFAULT, NULL, 0, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        RZ_TODO("report error");
        rz_return_defer(false);
    }

    rz_usize required_len = RZ_USIZE_MAX;
    while (true) {

        required_len = GetFinalPathNameByHandleA(h, buf, buf_len, VOLUME_NAME_DOS);
        if (required_len == 0) break;
        if (required_len < buf_len) {
            buf_len = required_len;
            break;
        } else {
            rz_usize new_len = required_len + 1;
            buf              = rz_raw_remap(a, buf, buf_len, new_len);
            buf_len          = new_len;
            continue;
        }
    }
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    if (required_len == 0) rz_return_defer(false);
#else
    // char *realpath(const char *restrict path, char *restrict resolved_path);
    if (realpath(cstr_path, buf) == NULL) {
        RZ_TODO("report error in");
        return false;
    }
    buf_len = strlen(ret);
#endif

defer:
    if (result) {
        path->len = 0;
        rz_str_append_sized_str(path, buf, buf_len);
        rz_str_append_null(path);
    }

    rz_temp_rewind(save);
    return result;
}

RZ_DEF bool rz_pathbuf_absolute(RZ_PathBuf *path) {
    if (rz_arr_is_empty(path)) {
        RZ_TODO("report error");
        return false;
    }

    auto ref = rz_path_from_pathbuf(path);
    if (rz_path_is_absolute(ref) || rz__path_is_verbatim(ref)) {
        // Verbatim paths should not be modified.
        return true;
    }

#if defined(RZ_OS_WINDOWS)
    auto        save      = rz_temp_snapshot();
    auto        a         = rz_temp_allocator();
    bool        result    = true;
    const char *path_cstr = rz_pathbuf_cstr(path);
    rz_usize    buf_len   = RZ_PATH_MAX;
    char       *buf       = rz_alloc_bytes(a, buf_len);

    while (true) {
        rz_usize required_len = GetFullPathNameA(path_cstr, buf_len, buf, NULL);
        if (required_len == 0) {
            rz_return_defer(false);
        } else if (required_len < buf_len) {
            buf_len = required_len;
            break;
        } else {
            rz_usize new_len = required_len + 1;
            buf              = rz_raw_remap(a, buf, buf_len, new_len);
            buf_len          = new_len;
            continue;
        }
    }

defer:
    if (result) {
        path->len = 0;
        rz_str_append_sized_str(path, buf, buf_len);
        rz_str_append_null(path);
    }

    rz_temp_rewind(save);
    return result;
#else
    // skip redundant './' or '.\\'
    if ((rz_arr_first(&ref) == '.') && !rz_sv_starts_with(ref, rz_sv_static(".."))) {
        ref.data += 1;
        ref.len -= 1;
        if (ref.len && ref.data[0] == RZ_PATH_SEP) {
            ref.data += 1;
            ref.len -= 1;
        }
    }
    RZ_PathBuf n = {.allocator = path->allocator};
    if (!rz_path_is_absolute(&ref))
        if (!rz_path_current_dir(&n)) return false;
    rz_pathbuf_push(&n, ref);
    return true;
#endif
}

RZ_DEF bool rz_path_current_dir(RZ_PathBuf *path) {
#if defined(RZ_OS_WINDOWS)
    rz_usize required_len = GetCurrentDirectoryA(path->len, NULL);
    if (required_len == 0) {
        // error
        return false;
    }
    rz_arr_resize(path, required_len + 1);

    required_len = GetCurrentDirectoryA(path->len, path->data);
    if (required_len == 0) {
        // error
        return false;
    }
#else
    rz_arr_reserve(path, PATH_MAX);
    if (getcwd(path->data, PATH_MAX) == NULL) {
        // error
        return false;
    }
#endif
    path->len = rz_strlen(path->data);
    return true;
}

RZ_DEF bool rz_path_current_exe(RZ_PathBuf *path) {
    RZ_UNUSED(path);
    RZ_TODO("rz_path_current_exe");
}

RZ_DEF bool rz_path_home_dir(RZ_PathBuf *path) {
    RZ_UNUSED(path);
    RZ_TODO("rz_path_home_dir");
}

RZ_DEF bool rz_path_temp_dir(RZ_PathBuf *path) {
    RZ_UNUSED(path);
    RZ_TODO("rz_path_temp_dir");
}

RZ_DEF bool rz_path_set_current_dir(const RZ_Path path) {
    RZ_UNUSED(path);
    RZ_TODO("rz_path_set_current_dir");
}

RZ_DEF const char *rz_path_cstr(const RZ_Path path, RZ_Allocator allocator) {
    if (rz_arr_is_empty(&path)) return rz_strdup(allocator, "");
    char *p = rz_memdup(allocator, path.data, path.len + 1);
    if (p == NULL) return NULL;
    p[path.len] = '\0';
    return p;
}

RZ_DEF bool rz_path_filename(const RZ_Path path, RZ_Path *filename) {
    RZ_ASSERT(filename != NULL);
    if (rz_arr_is_empty(&path)) return false;

    RZ_Path p = path;
    while (rz_sv_rsplit_char(&p, RZ_PATH_SEP, filename)) {
        // if empty its mean the end is '/'. continue until the path is empty
        if (rz_arr_is_empty(filename) || rz_sv_eq(*filename, rz_sv_static("."))) continue;

        // if the filename is '..' . its mean.
        // the function like '/this/is/path/../' or 'relative/path/..'
        if (rz_sv_eq(*filename, rz_sv_static(".."))) return false;
    }

    // if the filename is empty.
    // its mean the path is just '/'. and there is no filename
    if (rz_arr_is_empty(filename)) return false;
    return true;
}

RZ_DEF bool rz_path_filestem(const RZ_Path path, RZ_Path *filestem) {
    if (!rz_path_filename(path, filestem)) return false;

    rz_usize dot = RZ_NOT_FOUND;
    if ((dot = rz_sv_rfind_char(path, '.')) == RZ_NOT_FOUND) {
        // if not found dot '.'. its mean there is no extension.
        // so its just found a stem. and return true
        return true;
    }

    auto ret = rz_sv_strip_nsuffix(filestem, dot);
    RZ_UNUSED(ret);
    return true;
}

RZ_DEF bool rz_path_extension(const RZ_Path path, RZ_Path *extension) {
    RZ_Path filename = {0};
    if (!rz_path_filename(path, &filename)) return false;
    if (rz_sv_rsplit_char(&filename, '.', extension)) {
        // if success rsplit to get the extesion. return true
        // but if the result after split filename part is empty, return false.
        // its mean the filename only have dot in start of filename.
        //      filename = '.filaname'.
        // but if filename has dot on end of filename.
        // its still return true but the extension is empty
        //      filename = 'filename.'
        return !rz_arr_is_empty(&filename);
    }
    return false;
}

RZ_DEF bool rz_path_has_extension(const RZ_Path path, RZ_Path ext) {
    RZ_Path path_ext = {0};
    if (!rz_path_extension(path, &path_ext)) return false;
    return rz_sv_eq(path_ext, ext);
}

RZ_DEF bool rz_path_is_absolute(const RZ_Path path) {
#if defined(RZ_OS_WINDOWS)
    return (rz__path_is_verbatim(path) || rz__path_is_unc(path) || rz__path_is_drive_absolute_strict(path) || rz__path_is_device_namespace(path));
#else
    return rz__path_is_rooted_current_drive(path);
#endif
}

RZ_DEF bool rz_path_is_relative(const RZ_Path path) {
    return !rz_path_is_absolute(path);
}

#ifdef RZ_OS_WINDOWS
static RZ_PathFileType rz__path_filetype(DWORD attr, DWORD reparse) {
    bool isdir     = rz_bit(attr, FILE_ATTRIBUTE_DIRECTORY);
    bool issymlink = rz_bit(attr, FILE_ATTRIBUTE_REPARSE_POINT) && rz_bit(reparse, 0x20000000);
    if (issymlink) {
        return RZ_PATH_FILE_TYPE_SYMLINK;
    } else if (isdir && !issymlink) {
        return RZ_PATH_FILE_TYPE_DIR;
    } else if (!isdir && !issymlink) {
        return RZ_PATH_FILE_TYPE_FILE;
    } else {
        return RZ_PATH_FILE_TYPE_UNKNOWN;
    }
}

static void rz__path_metadata_from_find_data(RZ_PathMetadata *m, WIN32_FIND_DATAA wfd) {
    auto reparse_tag = (rz_bit(wfd.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT) ? wfd.dwReserved0 : 0);
    m->type          = rz__path_filetype(wfd.dwFileAttributes, reparse_tag);
    m->size          = ((rz_u64)wfd.nFileSizeLow) | (((rz_u64)wfd.nFileSizeHigh) << 32);
    m->accessed      = rz_systemtime_from_os(wfd.ftLastAccessTime);
    m->created       = rz_systemtime_from_os(wfd.ftCreationTime);
    m->modified      = rz_systemtime_from_os(wfd.ftLastWriteTime);
}

static bool rz__path_metadata(const char *path, bool symlink, RZ_PathMetadata *m) {
    RZ_DBG_ASSERT(path != NULL && m != NULL);
    bool result    = true;

    DWORD  reparse = (symlink) ? FILE_FLAG_OPEN_REPARSE_POINT : 0;
    HANDLE h       = CreateFileA(path, 0, RZ__FILE_SHARE_MODE_DEFAULT, NULL, 0, FILE_FLAG_BACKUP_SEMANTICS | reparse, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        auto err = GetLastError();
        if ((err == ERROR_SHARING_VIOLATION) || (err == ERROR_ACCESS_DENIED)) {
            // `ERROR_ACCESS_DENIED` is returned when the user doesn't have permission for the resource.
            // One such example is `System Volume Information` as default but can be created as well
            // `ERROR_SHARING_VIOLATION` will almost never be returned.
            // Usually if a file is locked you can still read some metadata.
            // However, there are special system files, such as
            // `C:\hiberfil.sys`, that are locked in a way that denies even that.
            WIN32_FIND_DATAA wfd = {0};
            // `FindFirstFileExW` accepts wildcard file names.
            // Fortunately wildcards are not valid file names and
            // `ERROR_SHARING_VIOLATION` means the file exists (but is locked)
            // therefore it's safe to assume the file name given does not
            // include wildcards.
            HANDLE fh = FindFirstFileExA(path, FindExInfoBasic, &wfd, FindExSearchNameMatch, NULL, 0);
            if (fh == INVALID_HANDLE_VALUE) {
                rz_return_defer(false);
            } else {
                FindClose(fh);

                rz__path_metadata_from_find_data(m, wfd);
                if ((!symlink) && rz_path_filetype_is_symlink(m->type)) rz_return_defer(false);
                else rz_return_defer(true);
            }
        }
        rz_return_defer(false);
    }

    BY_HANDLE_FILE_INFORMATION info = {0};
    if (!GetFileInformationByHandle(h, &info)) rz_return_defer(false);
    DWORD reparse_tag = 0;
    if (rz_bit(info.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT)) {
        FILE_ATTRIBUTE_TAG_INFO attr_tag = {0};
        if (!GetFileInformationByHandleEx(h, FileAttributeTagInfo, &attr_tag, sizeof(attr_tag))) return false;
        if ((attr_tag.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
            reparse_tag = attr_tag.ReparseTag;
        }
    }
    m->type     = rz__path_filetype(info.dwFileAttributes, reparse_tag);
    m->size     = ((rz_u64)info.nFileSizeLow) | (((rz_u64)info.nFileSizeHigh) << 32);
    m->accessed = rz_systemtime_from_os(info.ftLastAccessTime);
    m->created  = rz_systemtime_from_os(info.ftCreationTime);
    m->modified = rz_systemtime_from_os(info.ftLastWriteTime);

defer:
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    return result;
}
#else

static RZ_PathFileType rz__path_filetype(mode_t mode) {
    switch (mode & S_IFMT) {
    case S_IFDIR:
        return RZ_PATH_FILE_TYPE_DIR;
    case S_IFLNK:
        return RZ_PATH_FILE_TYPE_SYMLINK;
    case S_IFREG:
        return RZ_PATH_FILE_TYPE_FILE;
    default:
        return RZ_PATH_FILE_TYPE_UNKNOWN;
    }
}

static inline bool rz__path_metadata(const char *path, bool symlink, RZ_PathMetadata *m) {
    struct stat s = {0};
    if (((symlink) ? lstat(path, &s) : stat(path, &s)) != 0) {
        // TODO: report error
        return false;
    }
    m->size = (rz_u64)s.st_size;
    m->type = rz__path_filetype(s.st_mode);
#    if defined(RZ_OS_NETBSD)
    m->modified = rz_systemtime_from(s.st_mtime, s.st_mtimensec);
    m->accessed = rz_systemtime_from(s.st_atime, s.st_atimensec);
    m->created  = rz_systemtime_from(s.st_birthtime, s.st_birthtimensec);
#    else
    m->modified = rz_systemtime_from(s.st_mtime, s.st_mtime_nsec);
    m->accessed = rz_systemtime_from(s.st_atime, s.st_atime_nsec);
#        if (defined(RZ_OS_FREEBSD) || defined(RZ_OS_OPENBSD) || defined(RZ_OS_APPLE))
    m->created = rz_systemtime_from(s.st_birthtime, s.st_birthtime_nsec);
#        endif
#    endif
}
#endif

RZ_DEF bool rz_path_metadata(const RZ_Path path, RZ_PathMetadata *metadata) {
    auto        save      = rz_temp_snapshot();
    const char *path_cstr = rz_path_cstr_temp(path);
    bool        result    = rz__path_metadata(path_cstr, false, metadata);
    rz_temp_rewind(save);
    return result;
}

RZ_DEF bool rz_path_symlink_metadata(const RZ_Path path, RZ_PathMetadata *metadata) {
    auto        save      = rz_temp_snapshot();
    const char *path_cstr = rz_path_cstr_temp(path);
    bool        result    = rz__path_metadata(path_cstr, true, metadata);
    rz_temp_rewind(save);
    return result;
}

#define rz__path_metadata_check_access(path, access, onerror)                                      \
    RZ_PathMetadata metadata = {0};                                                                \
    if (!rz_path_metadata(path, &metadata)) {                                                      \
        /*RZ_LOG_ERR("Failed to get metadata for file: "RZ_SVFmt". %s",RZ_SVArg() rz_strerror())*/ \
        return onerror;                                                                            \
    }                                                                                              \
    return metadata.access;

RZ_DEF RZ_PathFileType rz_path_filetype(const RZ_Path path) {
    rz__path_metadata_check_access(path, type, RZ_PATH_FILE_TYPE_UNKNOWN);
}

RZ_DEF rz_u64 rz_path_file_size(const RZ_Path path) {
    rz__path_metadata_check_access(path, size, -1);
}

RZ_DEF RZ_SystemTime rz_path_modified(const RZ_Path path) {
    rz__path_metadata_check_access(path, modified, RZ_TIME_UNIX_EPOCH);
}

RZ_DEF RZ_SystemTime rz_path_accessed(const RZ_Path path) {
    rz__path_metadata_check_access(path, accessed, RZ_TIME_UNIX_EPOCH);
}

RZ_DEF RZ_SystemTime rz_path_created(const RZ_Path path) {
    rz__path_metadata_check_access(path, created, RZ_TIME_UNIX_EPOCH);
}

#undef rz__path_metadata_check_access

RZ_DEF bool rz_path_exists(const RZ_Path path) {
    bool res = false;
    auto s   = rz_temp_snapshot();
#if defined(RZ_OS_WINDOWS)
    res = GetFileAttributesA(rz_path_cstr_temp(path)) != INVALID_FILE_ATTRIBUTES;
#else
    res = access(rz_path_cstr_temp(path), F_OK) == 0;
#endif
    rz_temp_rewind(s);
    return res;
}

RZ_DEF bool rz_path_read_symlink(const RZ_Path path, RZ_PathBuf *symlink_result) {
    bool        result    = false;
    auto        s         = rz_temp_snapshot();
    auto        ta        = rz_temp_allocator();
    const char *path_cstr = rz_path_cstr_temp(path);
#if defined(RZ_OS_WINDOWS)
    // HANDLE h = CreateFileA(path_cstr, 0, RZ__FILE_SHARE_MODE_DEFAULT, NULL, 0, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    // if (h == INVALID_HANDLE_VALUE) {
    //     RZ_TODO("report error");
    //     rz_return_defer(false);
    // }
    // char *space = rz_alloc_bytes(a, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

    // defer:
    //     if (h != INVALID_HANDLE_VALUE) CloseHandle(h);

    RZ_UNUSED(symlink_result);
    RZ_UNUSED(ta);
    RZ_UNUSED(path_cstr);
    RZ_UNIMPLEMENTED("rz_path_read_symlink (on Windows). too complicated. implement later when in mood. XD");
#else
    symlink_result->len = 0;
    if (symlink_result->capacity == 0) {
        rz_arr_reserve(symlink_result, 256);
    }
    while (true) {
        auto r = readlink(path_cstr, symlink_result->data, symlink_result->capacity);
        if (r < 0) {
            // TODO: report error?
            result = false;
            break;
        }
        if (r < RZ_PATH_MAX) {
            symlink_result->len = r;
            result              = true;
            break;
        }
        rz_arr_reserve(symlink_result, symlink_result->capacity * 2);
    }
#endif
    rz_temp_rewind(s);
    return result;
}

RZ_DEF RZ_PathModifiedStatus rz_path_is_modified(const RZ_Path input, const RZ_Path output) {
    RZ_PathMetadata m_input  = {0};
    RZ_PathMetadata m_output = {0};
    if (!rz_path_metadata(input, &m_input)) {
        // report error?
        return RZ_PATH_MODIF_ERROR;
    }
    if (!rz_path_metadata(output, &m_output)) {
        // report error?
        return RZ_PATH_MODIF_ERROR;
    }

    // if time modified of input is greater than outout. its mean the input is modified
    if (rz_duration_cmp(m_input.modified.t, m_output.modified.t) > 0) {
        return RZ_PATH_MODIF_CHANGED;
    } else {
        return RZ_PATH_MODIF_UNCHANGED;
    }
}

RZ_DEF RZ_Fd rz_fd_open_opt(const RZ_Path *path, RZ_FdOpenOpt opt) {
#if defined(RZ_OS_WINDOWS)
    DWORD desiredAccess       = 0;
    DWORD creationDisposition = 0;
    DWORD flagsAndAttributes  = FILE_ATTRIBUTE_NORMAL;
    DWORD shareMode           = RZ__FILE_SHARE_MODE_DEFAULT; // allow others to read/write unless you want exclusive

    if (opt.read) desiredAccess |= GENERIC_READ;
    if (opt.write) desiredAccess |= GENERIC_WRITE;

    if (opt.create && opt.truncate) {
        creationDisposition = CREATE_ALWAYS;     // create or overwrite
    } else if (opt.create) {
        creationDisposition = OPEN_ALWAYS;       // open if exists, else create
    } else if (opt.truncate) {
        creationDisposition = TRUNCATE_EXISTING; // must exist
    } else {
        creationDisposition = OPEN_EXISTING;
    }

    if (opt.append) {
        // For append semantics, still open with write access; callers should SetFilePointer to end before writes.
        // Keep creationDisposition as chosen above.
        // No special flag needed here, but ensure GENERIC_WRITE is present.
        desiredAccess |= GENERIC_WRITE;
    }

    if (!opt.read && opt.write) {
        // if write-only, allow read-sharing by others; keep attributes normal
    } else if (opt.read && !opt.write) {
        // read-only open
        flagsAndAttributes = FILE_ATTRIBUTE_READONLY;
    }

    SECURITY_ATTRIBUTES saAttr;
    ZeroMemory(&saAttr, sizeof(saAttr));
    saAttr.nLength              = sizeof(saAttr);
    saAttr.bInheritHandle       = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE handle               = CreateFileA(path->data, desiredAccess, shareMode, &saAttr, creationDisposition, flagsAndAttributes, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        // map Win32 error to your rz error handling if desired
        return RZ_INVALID_FD;
    }

    // If opened with append semantics, move pointer to end.
    if (opt.append) {
        SetFilePointer(handle, 0, NULL, FILE_END);
    }

    return (RZ_Fd)handle;
#else // unix

    RZ_Fd fd = RZ_INVALID_FD;

    int flag = 0;
    if (opt.read) flag |= O_RDONLY;
    if (opt.write) flag |= O_WRONLY;
    if (opt.append) flag |= O_APPEND;
    if (opt.create) flag |= O_CREAT;
    if (opt.create_new) flag |= O_CREAT;
    if (opt.truncate) flag |= O_TRUNC;

    if (opt.create || opt.create_new || opt.write) {
        if (opt.permission == 0) opt.permission = RZ_DEFAULT_OPEN_CREATION_PERMISION;
        fd = open(path.data, flag, (mode_t)opt.permission);
    } else {
        fd = open(path.data, flag);
    }

    if (fd < 0) return RZ_INVALID_FD;
    return fd;
#endif
}

RZ_DEF bool rz_fd_close(RZ_Fd fd) {
#if defined(RZ_OS_WINDOWS)
    return CloseHandle(fd);
#else
    return close(fd) != 0;
#endif // _WIN32
}

RZ_DEC bool rz_fd_read(RZ_Fd fd, void *dst, rz_usize dstsize, rz_usize *readsize) {
#if defined(RZ_OS_WINDOWS)
    if (!ReadFile(fd, dst, dstsize, (LPDWORD)readsize, NULL)) {
        RZ_OS_ERROR(RZ_TAG, "Failed to read file.");
        return false;
    }
    return true;
#else
    *readsize = read(fd, dst, dstsize);
    if (*readsize < 0) {
        RZ_OS_ERROR(RZ_TAG, "Failed to read file.");
        return false;
    }
    return true;
#endif
}

RZ_DEC bool rz_fd_write(RZ_Fd fd, const void *src, rz_usize srclen, rz_usize *writesize) {
#if defined(RZ_OS_WINDOWS)
    if (!WriteFile(fd, src, srclen, (LPDWORD)writesize, NULL)) {
        RZ_OS_ERROR(RZ_TAG, "Failed to write file.");
        return false;
    }
    return true;
#else
    *writesize = write(fd, src, srclen);
    if (*writesize < 0) {
        RZ_OS_ERROR(RZ_TAG, "Failed to read file.");
        return false;
    }
    return true;
#endif
}

RZ_DEC bool rz_fd_read_all(RZ_Fd fd, RZ_StrBuilder *sb) {
    char     buf[4 * 1024] = {0};
    rz_usize readsize      = 0;
    while (true) {
        if (!rz_fd_read(fd, buf, sizeof(buf), &readsize)) return false;
        if (readsize == 0) break;
        rz_str_append_sized_str(sb, buf, readsize);
    }
    return true;
}

RZ_DEC bool rz_fd_write_all(RZ_Fd fd, const char *src, rz_usize srclen) {
    const char *s        = src;
    rz_usize    writelen = 0;
    while (srclen > 0) {
        if (!rz_fd_write(fd, s, srclen, &writelen)) return false;

        s += writelen;
        srclen -= writelen;
    }
    return true;
}

RZ_DEF RZ_Fd rz_fd_stdin(void) {
#if defined(RZ_OS_WINDOWS)
    return GetStdHandle(STD_INPUT_HANDLE);
#else
    return STDIN_FILENO;
#endif // _WIN32
}

RZ_DEF RZ_Fd rz_fd_stdout(void) {
#if defined(RZ_OS_WINDOWS)
    return GetStdHandle(STD_OUTPUT_HANDLE);
#else
    return STDOUT_FILENO;
#endif // _WIN32
}

RZ_DEF RZ_Fd rz_fd_stderr(void) {
#if defined(RZ_OS_WINDOWS)
    return GetStdHandle(STD_ERROR_HANDLE);
#else
    return STDERR_FILENO;
#endif // _WIN32
}

RZ_DEC bool rz_fd_pipe(RZ_FdPipe *pp) {
    RZ_DBG_ASSERT_NOT_NULL(pp);
#ifdef RZ_OS_WINDOWS
    // https://docs.microsoft.com/en-us/windows/win32/ProcThread/creating-a-child-process-with-redirected-input-and-output
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength             = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle      = TRUE;

    if (!CreatePipe(&pp->read, &pp->write, &saAttr, 0)) {
        // rz_log(RZ_LOG_ERROR, "Could not create pipe: %s", rz_strerror());
        return false;
    }

    return true;
#else
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        // rz_log(RZ_LOG_ERROR, "Could not create pipe: %s\n", rz_strerror());
        return false;
    }

    pp->read  = pipefd[0];
    pp->write = pipefd[1];

    return true;
#endif // _WIN32
}

RZ_DEF bool rz_getenv(const char *name, RZ_Str *res) {
    RZ_DBG_ASSERT(name != NULL && res != NULL);
    res->len = 0;
#if defined(RZ_OS_WINDOWS)
    if (res->capacity == 0) {
        rz_arr_reserve(res, 512);
    }
    DWORD ret_size = 0;
    while (true) {
        if ((ret_size = GetEnvironmentVariableA(name, res->data, res->capacity)) == 0) {
            RZ_OS_ERROR(RZ_TAG, "failed to get environment variable");
            return false;
        }
        if ((ret_size == res->capacity) && (errno == ERROR_INSUFFICIENT_BUFFER)) {
            rz_arr_reserve(res, res->capacity * 2);
        } else if (ret_size > res->capacity) {
            rz_arr_reserve(res, ret_size);
        } else {
            res->len = ret_size;
            break;
        }
    }
#else
    const char *v = getenv(name);
    if (v == NULL) {
        RZ_OS_ERROR(RZ_TAG, "failed to get environment variable");
        return false;
    }
    rz_str_append_cstr(res, v);
#endif
    return true;
}

struct RZ_PathDir {

#if defined(RZ_OS_WINDOWS)
    WIN32_FIND_DATA win32_data;
    HANDLE          win32_hFind;
    bool            win32_init;
#else
    DIR           *posix_dir;
    struct dirent *posix_ent;
#endif // _WIN32
};

RZ_DEC RZ_PathDir *rz_dir_open(const RZ_Path path, RZ_Allocator a) {
    RZ_PathDir *res = rz_calloc(a, res, 1);
    RZ_ASSERT_ALLOCATOR_PTR(res);

#if defined(RZ_OS_WINDOWS)
    RZ_TEMP_ALLOCATOR_BLOCK(a, {
        char *buffer     = rz_asprintf(a, RZ_SVFmt "\\*", RZ_SVArg(path));
        res->win32_hFind = FindFirstFileA(buffer, &res->win32_data);
    });

    if (res->win32_hFind == INVALID_HANDLE_VALUE) {
        RZ_OS_ERROR(RZ_TAG, "Could not open directory '" RZ_SVFmt "'", RZ_SVArg(path));
        rz_free(a, res, 1);
        return NULL;
    }
#else
    RZ_TEMP_ALLOCATOR_BLOCK(a, {
        char *buffer   = rz_path_cstr(path, a);
        res->posix_dir = opendir(dir_path);
    });
    if (res->posix_dir == NULL) {
        RZ_OS_ERROR(RZ_TAG, "Could not open directory '" RZ_SVFmt "'", RZ_SVArg(path));
        rz_free(a, res, 1);
        return NULL;
    }
#endif
    return res;
}
RZ_DEC bool rz_dir_next(RZ_PathDir *dir, RZ_PathDirEntry *entry) {
#if defined(RZ_OS_WINDOWS)
    if (!dir->win32_init) {
        dir->win32_init = true;
        entry->filename = dir->win32_data.cFileName;
        return true;
    }

    if (!FindNextFile(dir->win32_hFind, &dir->win32_data)) {
        if (rz_last_os_error() == ERROR_NO_MORE_FILES) return false;
        RZ_OS_ERROR(RZ_TAG, "Could not read next directory entry");
        entry->filename = NULL;
        return false;
    }
    entry->filename  = dir->win32_data.cFileName;
    auto reparse_tag = (rz_bit(dir->win32_data.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT) ? dir->win32_data.dwReserved0 : 0);
    entry->type      = rz__path_filetype(dir->win32_data.dwFileAttributes, reparse_tag);
#else

    errno          = 0;
    dir->posix_ent = readdir(dir->posix_dir);
    if (dir->posix_ent == NULL) {
        if (rz_errno == 0) return false;
        RZ_OS_ERROR(RZ_TAG, "Could not read next directory entry");
        entry->filename = NULL;
        return false;
    }
    entry->name = dir->posix_ent->d_name;
    switch (dir->posix_ent->d_type) {
    case DT_DIR:
        entry->type = RZ_PATH_FILE_TYPE_DIR;
        break;
    case DT_LNK:
        entry->type = RZ_PATH_FILE_TYPE_SYMLINK;
        break;
    case DT_UNKNOWN:
        entry->type = RZ_PATH_FILE_TYPE_UNKNOWN;
        break;
    default:
        entry->type = RZ_PATH_FILE_TYPE_FILE;
        break;
    }
#endif
    return true;
}
RZ_DEC void rz_dir_close(RZ_PathDir *dir, RZ_Allocator a) {
    if (dir == NULL) return;
#ifdef _WIN32
    FindClose(dir->win32_hFind);
#else
    if (dir->posix_dir) closedir(dir->posix_dir);
#endif
    rz_free(a, dir, 1);
}

#undef RZ_TAG
