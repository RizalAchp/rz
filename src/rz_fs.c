#include "rz_fs.h"
#include "rz_allocator.h"
#include "rz_collections.h"
#include "rz_common.h"
#include "rz_sprintf.h"

#include "rz_error.h"
#include "rz_logger.h"

#define RZ_TAG "rz_fs"

RZ_ALWAYS_INLINE static bool rz__path_is_sep(char ch) {
    return ((ch == RZ_PATH_WINDOWS_SEP) || (ch == RZ_PATH_UNIX_SEP));
}
RZ_ALWAYS_INLINE static bool rz__path_is_verbatim(const RZ_Path path) {
    return (rz_sv_starts_with(path, rz_sv_static("\\\\?\\")) || rz_sv_starts_with(path, rz_sv_static("\\??\\")));
}
RZ_ALWAYS_INLINE static bool rz__path_is_rooted_current_drive(RZ_Path path) {
    return (path.len > 0) && rz__path_is_sep(path.data[0]);
}
#if RZ_TARGET_OS_WINDOWS
/* --- Windows path predicates --- */
#    define RZ__FILE_SHARE_MODE_DEFAULT FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE
RZ_ALWAYS_INLINE static bool rz__path_is_drive_letter(const RZ_Path path) {
    return (path.len < 2) && ((rz_in_range(path.data[0], 'A', 'Z') || rz_in_range(path.data[0], 'a', 'z')) || (path.data[1] == ':'));
}
RZ_ALWAYS_INLINE static bool rz__path_is_drive_absolute_strict(const RZ_Path path) {
    return ((path.len >= 3) && rz__path_is_drive_letter(path) && rz__path_is_sep(path.data[2]));
}
RZ_ALWAYS_INLINE static bool rz__path_is_unc(const RZ_Path path) {
    return rz_sv_starts_with(path, rz_sv_static("\\\\"));
}
RZ_ALWAYS_INLINE static bool rz__path_is_device_namespace(RZ_Path path) {
    return rz_sv_starts_with(path, rz_sv_static("\\\\.\\"));
}
#endif

RZ_DEF const char *rz_pathbuf_cstr(RZ_PathBuf *path) {
    RZ_DBG_ASSERT_NOT_NULL(path);
    if (path->len < path->capacity) {
        auto end = rz_arr_end(path);
        if (*end == '\0') {
            return path->data;
        } else {
            *end = '\0';
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

    auto    save      = rz_temp_snapshot();
    RZ_Path extension = {0};
    if (!rz_arr_is_empty(&filename)) {
        // if path contains filename. add the extension to the path again.
        // find the last dot.
        rz_usize dot = rz_sv_rfind_char(filename, '.');
        if (dot != RZ_NOT_FOUND) {
            RZ_StrView t = rz_sv_strip_nsuffix(&filename, dot);
            // to prevent overwrite of extension. duplicate using temp allocator.
            extension = (RZ_StrView){
                .data = rz_tmemdup(t.data, t.len),
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

    rz_temp_rewind(save);
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

RZ_DEF RZ_PathBuf rz_pathbuf_join(const RZ_PathBuf *path, RZ_Path component) {
    RZ_PathBuf res = rz_pathbuf_clone(path);
    rz_pathbuf_push(&res, component);
    return res;
}

static void rz__path_change_sep(RZ_Path *path) {
    rz_foreach(ch, path) {
#if RZ_TARGET_OS_WINDOWS
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

#define rz__path_cstr_temp(path) rz_path_cstr(path, rz_temp_allocator())

RZ_DEF const char *rz_path_cstr(const RZ_Path path, RZ_Allocator allocator) {
    if (rz_arr_is_empty(&path)) return rz_strdup(allocator, "");
    char *p = rz_alloc_bytes(allocator, path.len + 1);
    RZ_ASSERT_ALLOCATOR_PTR(p);
    memcpy(p, path.data, path.len);
    p[path.len] = '\0';
    return p;
}

RZ_DEF bool rz_path_filename(const RZ_Path path, RZ_Path *filename) {
    RZ_ASSERT(filename != NULL);
    if (rz_arr_is_empty(&path)) return false;

    RZ_Path p = path;
    while (rz_sv_rsplit_by(&p, rz__path_is_sep, filename)) {
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

RZ_DEF bool rz_path_parent_dir(const RZ_Path path, RZ_Path *parent_dir) {
    RZ_DBG_ASSERT(parent_dir != NULL);
    RZ_Path right = {0};
    *parent_dir   = path;
    while (rz_sv_rsplit_by(parent_dir, rz__path_is_sep, &right)) {
        // if like this. the right is empty. ex: "/this/is/path/" "/this/is/path/."
        if (rz_sv_is_empty(right) || rz_sv_eq(right, rz_sv_static("."))) continue;
        break;
    }
    if (rz_sv_is_empty(*parent_dir)) return rz_path_is_relative(path);
    return true;
}

RZ_DEF bool rz_path_is_absolute(const RZ_Path path) {
#if RZ_TARGET_OS_WINDOWS
    return (rz__path_is_verbatim(path) || rz__path_is_unc(path) || rz__path_is_drive_absolute_strict(path) || rz__path_is_device_namespace(path));
#else
    return rz__path_is_rooted_current_drive(path);
#endif
}

RZ_DEF bool rz_path_is_relative(const RZ_Path path) {
    return !rz_path_is_absolute(path);
}

#if RZ_TARGET_OS_WINDOWS
static RZ_PathFileType rz__path_filetype(DWORD attr, DWORD reparse) {
    RZ_PathFileType type = RZ_PATH_FILE_TYPE_FILE;
    if (rz_bit_all(attr, FILE_ATTRIBUTE_REPARSE_POINT, 0x20000000)) rz_bit_set(type, RZ_PATH_FILE_TYPE_SYMLINK);
    if (rz_bit(attr, FILE_ATTRIBUTE_DIRECTORY)) rz_bit_set(type, RZ_PATH_FILE_TYPE_DIR);
    if (rz_bit(attr, FILE_ATTRIBUTE_HIDDEN)) rz_bit_set(type, RZ_PATH_FILE_TYPE_HIDDEN);
    return type;
}

static void rz__path_metadata_from_find_data(RZ_PathMetadata *m, WIN32_FIND_DATAA wfd) {
    auto reparse_tag = (rz_bit(wfd.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT) ? wfd.dwReserved0 : 0);
    m->type          = rz__path_filetype(wfd.dwFileAttributes, reparse_tag);
    m->size          = ((rz_u64)wfd.nFileSizeLow) | (((rz_u64)wfd.nFileSizeHigh) << 32);
    m->accessed      = rz_systemtime_from_os(wfd.ftLastAccessTime);
    m->created       = rz_systemtime_from_os(wfd.ftCreationTime);
    m->modified      = rz_systemtime_from_os(wfd.ftLastWriteTime);
}
static bool rz__path_metadata_from_fd(RZ_Fd h, RZ_PathMetadata *m) {
    BY_HANDLE_FILE_INFORMATION info = {0};
    if (!GetFileInformationByHandle(h, &info)) return false;
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
    return true;
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
                if ((!symlink) && rz_filetype_is_symlink(m->type)) {
                    // should it be dereference the symlink?
                    RZ_ERROR("Try to get metadata but the file is symlink. (path: %s)", path);
                    rz_return_defer(false);
                } else {
                    rz_return_defer(true);
                }
            }
        }
        RZ_OS_ERROR_INTR("Failed to open path to get metadata. (path: %s)", path);
        rz_return_defer(false);
    }
    if (!rz__path_metadata_from_fd(h, m)) {
        RZ_OS_ERROR_INTR("Failed to open path to get metadata. (path: %s)", path);
        rz_return_defer(false);
    }
defer:
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    if (!symlink && !result && (rz_last_os_error() == ERROR_CANT_ACCESS_FILE)) {
        if (rz__path_metadata(path_cstr, !symlink, metadata)) {
            return !rz_filetype_is_symlink(metadata->type);
        }
    }
    return result;
}
#else

static RZ_PathFileType rz__path_filetype(mode_t mode) {
    RZ_PathFileType type = RZ_PATH_FILE_TYPE_FILE;
    if (S_ISDIR(mode)) {
        type |= RZ_PATH_FILE_TYPE_DIR;
    }
    if (S_ISLNK(mode)) {
        type |= RZ_PATH_FILE_TYPE_SYMLINK;
    }
    return type;
}

static void rz__path_metadata_from_stat(struct stat s, RZ_PathMetadata *m) {
    m->perm = s.st_mode;
    m->size = (rz_u64)s.st_size;
    m->type = rz__path_filetype(s.st_mode);
#    if RZ_TARGET_OS_NETBSD
    m->modified = rz_systemtime_from(s.st_mtime, s.st_mtimensec);
    m->accessed = rz_systemtime_from(s.st_atime, s.st_atimensec);
    m->created  = rz_systemtime_from(s.st_birthtime, s.st_birthtimensec);
#    elif defined(__USE_XOPEN2K8)
    m->modified = rz_systemtime_from(s.st_mtim.tv_sec, s.st_mtim.tv_nsec);
    m->accessed = rz_systemtime_from(s.st_atim.tv_sec, s.st_atim.tv_nsec);
    m->created  = rz_systemtime_from(s.st_ctim.tv_sec, s.st_ctim.tv_nsec);
#    else
    m->modified = rz_systemtime_from(s.st_mtime, s.st_mtimensec);
    m->accessed = rz_systemtime_from(s.st_atime, s.st_mtimensec);
    m->created  = rz_systemtime_from(s.st_ctime, s.st_ctimensec);
#    endif
}

static bool rz__path_metadata_from_fd(RZ_Fd fd, RZ_PathMetadata *m) {
    struct stat s = {0};
    if (fstat(fd, &s) != 0) {
        RZ_OS_ERROR_INTR("Failed to get metadata from fd: %d", fd);
        return false;
    }
    rz__path_metadata_from_stat(s, m);
    return true;
}

static inline bool rz__path_metadata(const char *path, bool symlink, RZ_PathMetadata *m) {
    struct stat s = {0};
    if (((symlink) ? lstat(path, &s) : stat(path, &s)) != 0) {
        RZ_OS_ERROR_INTR("Failed to get metadata for (path: %s).", path);
        return false;
    }
    rz__path_metadata_from_stat(s, m);
    return true;
}
#endif

RZ_DEF bool rz_path_metadata(const RZ_Path path, RZ_PathMetadata *metadata) {
    auto m      = rz_temp_snapshot();
    auto result = rz__path_metadata(rz__path_cstr_temp(path), false, metadata);
    rz_temp_rewind(m);
    return result;
}

RZ_DEF bool rz_path_symlink_metadata(const RZ_Path path, RZ_PathMetadata *metadata) {
    auto m      = rz_temp_snapshot();
    auto result = rz__path_metadata(rz__path_cstr_temp(path), true, metadata);
    rz_temp_rewind(m);
    return result;
}

#define rz__path_metadata_check_access(path, access, onerror) \
    RZ_PathMetadata metadata = {0};                           \
    if (!rz_path_metadata(path, &metadata)) {                 \
        return onerror;                                       \
    }                                                         \
    return metadata.access;

RZ_DEF RZ_PathFileType rz_path_filetype(const RZ_Path path) {
    rz__path_metadata_check_access(path, type, RZ_PATH_FILE_TYPE_UNKNOWN);
}

RZ_DEF rz_u64 rz_path_file_size(const RZ_Path path) {
    rz__path_metadata_check_access(path, size, -1);
}

RZ_DEF RZ_SystemTime rz_path_modified(const RZ_Path path) {
    rz__path_metadata_check_access(path, modified, RZ_SYSTIME_FAILED);
}

RZ_DEF RZ_SystemTime rz_path_accessed(const RZ_Path path) {
    rz__path_metadata_check_access(path, accessed, RZ_SYSTIME_FAILED);
}

RZ_DEF RZ_SystemTime rz_path_created(const RZ_Path path) {
    rz__path_metadata_check_access(path, created, RZ_SYSTIME_FAILED);
}

#undef rz__path_metadata_check_access

RZ_DEF bool rz_fs_exists(const RZ_Path path) {
    auto m = rz_temp_snapshot();
#if RZ_TARGET_OS_WINDOWS
    bool result = GetFileAttributesA(rz__path_cstr_temp(path)) != INVALID_FILE_ATTRIBUTES;
#else
    bool result = access(rz__path_cstr_temp(path), F_OK) == 0;
#endif
    rz_temp_rewind(m);
    return result;
}

RZ_DEF bool rz_fs_read_symlink(const RZ_Path path, RZ_PathBuf *symlink_result) {
    bool        result    = true;
    auto        m         = rz_temp_snapshot();
    const char *path_cstr = rz__path_cstr_temp(path);
#if RZ_TARGET_OS_WINDOWS
    // HANDLE h = CreateFileA(path_cstr, 0, RZ__FILE_SHARE_MODE_DEFAULT, NULL, 0, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    // if (h == INVALID_HANDLE_VALUE) {
    //     RZ_TODO("report error");
    //     rz_return_defer(false);
    // }
    // char *space = rz_alloc_bytes(a, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

    // defer:
    //     if (h != INVALID_HANDLE_VALUE) CloseHandle(h);

    RZ_UNUSED(symlink_result);
    RZ_UNUSED(path_cstr);
    RZ_UNIMPLEMENTED("rz_path_read_symlink (on Windows). too complicated. implement later when in mood. XD");
#else
    rz_arr_reserve(symlink_result, PATH_MAX);
    auto r = readlink(path_cstr, symlink_result->data, symlink_result->capacity);
    if (r < 0) {
        RZ_OS_ERROR_INTR("Failed to read_symlink for (path: %s)", path_cstr);
        rz_return_defer(false);
    }
    symlink_result->len = r;
    rz_return_defer(true);
#endif
defer:
    rz_temp_rewind(m);
    return result;
}

RZ_DEF bool rz_fs_canonicalize(RZ_PathBuf *path) {
    bool        result    = true;
    const char *cstr_path = rz_pathbuf_cstr(path);
    char       *buf       = NULL;
    rz_usize    buf_len   = RZ_PATH_MAX;

    auto save             = rz_temp_snapshot();
    auto a                = rz_temp_allocator();
    buf                   = rz_alloc_bytes(a, buf_len);

#if RZ_TARGET_OS_WINDOWS
    HANDLE h = CreateFileA(cstr_path, 0, RZ__FILE_SHARE_MODE_DEFAULT, NULL, 0, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        RZ_OS_ERROR_INTR("Failed to open file (path: " RZ_SVFmt ")", RZ_SVArg(path));
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
defer:
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    if (required_len == 0) rz_return_defer(false);
#else
    // char *realpath(const char *restrict path, char *restrict resolved_path);
    if (realpath(cstr_path, buf) == NULL) {
        RZ_OS_ERROR_INTR("Failed to get realpath() for (path: " RZ_SVFmt ")", RZ_SVArg(*path));
        return false;
    }
    buf_len = strlen(buf);
#endif
    if (result) {
        path->len = 0;
        rz_str_append_sized_str(path, buf, buf_len);
        rz_str_append_null(path);
    }

    rz_temp_rewind(save);
    return result;
}

RZ_DEF bool rz_fs_absolute(RZ_PathBuf *path) {
    RZ_DBG_ASSERT_NOT_NULL(path);

    if (rz_arr_is_empty(path)) {
        errno = EINVAL; // invalid argument
        RZ_OS_ERROR_INTR("Try to get absolute path for empty path");
        return false;
    }

    auto ref = rz_path_from_pathbuf(path);
    if (rz_path_is_absolute(ref) || rz__path_is_verbatim(ref)) {
        // Verbatim paths should not be modified.
        return true;
    }

#if RZ_TARGET_OS_WINDOWS
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
    if (!rz_path_is_absolute(ref)) {
        if (!rz_fs_current_dir(&n)) return false;
        rz_pathbuf_push(&n, ref);
        rz_arr_free(path);
        *path = n;
    }
    return true;
#endif
}

RZ_DEF bool rz_fs_current_dir(RZ_PathBuf *path) {
#if RZ_TARGET_OS_WINDOWS
    rz_usize required_len = GetCurrentDirectoryA(path->capacity, NULL);
    if (required_len == 0) {
        RZ_OS_ERROR_INTR("Failed to get current_dir on (path: " RZ_SVFmt ")", RZ_SVArg(*path));
        return false;
    }
    rz_arr_resize(path, required_len + 1);

    required_len = GetCurrentDirectoryA(path->capacity, path->data);
    if (required_len == 0) {
        RZ_OS_ERROR_INTR("Failed to get current_dir on (path: " RZ_SVFmt ")", RZ_SVArg(*path));
        return false;
    }
#else
    rz_arr_reserve(path, 512);
    for (;;) {
        if (getcwd(path->data, path->capacity) != NULL) break;
        if (rz_last_os_error() != ERANGE) {
            RZ_OS_ERROR_INTR("Failed to get current_dir on (path: " RZ_SVFmt ")", RZ_SVArg(*path));
            return false;
        }
        rz_arr_reserve(path, path->capacity * 2);
    }
#endif
    path->len = strlen(path->data);
    return true;
}

RZ_DEF bool rz_fs_set_current_dir(const RZ_Path path) {
    auto m         = rz_temp_snapshot();
    bool result    = true;
    auto path_cstr = rz__path_cstr_temp(path);
#if RZ_TARGET_OS_WINDOWS
    result = SetCurrentDirectoryA(path_cstr);
#else
    result = chdir(path_cstr) == 0;
#endif
    if (!result) RZ_OS_ERROR_INTR("Failed to change directory to (path: %s)", path_cstr);
    rz_temp_rewind(m);
    return result;
}

RZ_DEF bool rz_fs_current_exe(RZ_PathBuf *path) {
#if RZ_TARGET_OS_WINDOWS
    if (rz_arr_is_empty(path)) rz_arr_reserve(path, 128);
    for (;;) {
        DWORD len = GetModuleFileNameA(NULL, path->data, path->capacity);
        if (len == 0) {
            if (rz_last_os_error() == ERROR_INSUFFICIENT_BUFFER) {
                rz_arr_reserve(path, 128);
                continue;
            }
            RZ_OS_ERROR_INTR("Failed to get current executable dir");
            return false;
        }
        path->len = len;
        break;
    }
#elif RZ_TARGET_ANY(OS, LINUX, ANDROID, NETBSD) || RZ_TARGET_COMPILER_EMSCRIPTEN
    return rz_fs_read_symlink(rz_path_static("/proc/self/exe"), path);

#elif RZ_TARGET_OS_FREEBSD
    rz_int   mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    rz_usize sz    = 0;
    if (sysctl(mib, RZ_ARRAY_LEN(mib), NULL, &sz, NULL, 0) < 0) goto failed;
    if (sz == 0) goto failed;

    rz_arr_reserve(sz);
    if (sysctl(mib, RZ_ARRAY_LEN(mib), path->data, &path->len, NULL, 0) < 0) goto failed;
    if (path->len == 0) goto failed;
    path->len--;
    return true;
failed:
    RZ_OS_ERROR_INTR("Failed to get current executable");
    return false;
#elif RZ_TARGET_OS_OPENBSD
    rz_int   mib[]    = {CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_ARGV};
    rz_usize argv_len = 0;
    if (sysctl(mib, RZ_ARRAY_LEN(mib), NULL, &argv_len, NULL, 0) < 0) goto failed;

    auto s                      = rz_temp_snapshot();

    RZ_Array(const char *) argv = {.allocator = rz_temp_allocator()};
    rz_arr_reserve(argv_len);
    if (sysctl(mib, RZ_ARRAY_LEN(mib), argv.data, &argv.len, NULL, 0) < 0) goto failed;
    if (argv.data[0] == NULL) goto failed;

    path->len = 0;
    rz_str_append_cstr(path, argv.data[0]);
    rz_temp_rewind(s);

    return rz_fs_canonicalize(path);
failed:
    rz_temp_rewind(s);
    RZ_OS_ERROR_INTR("Failed to get current executable");
    return false;

#elif RZ_TARGET_FAMILY_APPLE
    rz_u32 sz = 0;
    _NSGetExecutablePath(NULL, &sz);
    if (sz == 0) {
        RZ_OS_ERROR_INTR("Failed to get executable path");
        return false;
    }
    rz_arr_reserve(path, sz);
    if (_NSGetExecutablePath(path->data, &path->len) != 0) {
        RZ_OS_ERROR_INTR("Failed to get executable path");
        return false;
    }
    path->len--; // remove the null char
    return true;
#else
#    error rz_fs_current_exe is not implemented yet for this platform
#endif
}

RZ_DEF bool rz_fs_home_dir(RZ_PathBuf *path) {
    path->len = 0;
#if RZ_TARGET_OS_WINDOWS
    if (rz_getenv("USERPROFILE", path) && !rz_arr_is_empty(path)) return true;
    DWORD sz = 0;
    if (!GetUserProfileDirectoryA(GetCurrentProcessToken(), NULL, &sz)) goto failed;
    rz_arr_resize(path, sz);
    if (!GetUserProfileDirectoryA(GetCurrentProcessToken(), path->data, (LPDWORD)&path->size)) goto failed;
    path->size--;
    return true;
failed:
    RZ_OS_ERROR_INTR("Failed to get home directory");
    return false;
#else
    if (rz_getenv("HOME", path)) return true;
    // fallback
#    if (RZ_TARGET_FAMILY_APPLE && !RZ_TARGET_OS_MACOS) || RZ_TARGET_OS_ANDROID || RZ_TARGET_COMPILER_EMSCRIPTEN
    RZ_ERROR("this platform is not supported or not implemented to get home directory");
    return false;
#    else
    bool          result = true;
    auto          m      = rz_temp_snapshot();
    struct passwd p = {0}, *presult = NULL;
    auto          res = getpwuid_r(getuid(), &p, rz_alloc_bytes(rz_temp_allocator(), RZ_PATH_MAX), RZ_PATH_MAX, &presult);
    if ((res != 0) || presult == NULL) {
        RZ_OS_ERROR_INTR("Failed to get home directory");
        rz_return_defer(false);
    }
    rz_str_append_cstr(path, presult->pw_dir);
defer:
    rz_temp_rewind(m);
    return result;
#    endif
#endif
}

RZ_DEF bool rz_fs_temp_dir(RZ_PathBuf *path) {
    path->len = 0;
#if RZ_TARGET_OS_WINDOWS
    rz_arr_reserve(path, RZ_PATH_MAX + 1);
    for (;;) {
        auto required_len = GetTempPath2A(path->capacity, path->data);
        if (required_len == 0) {
            RZ_OS_ERROR_INTR("Failed to get temp directory");
            return false;
        }
        if (required_len > path->capacity) {
            rz_arr_reserve(path, required_len);
            path->capacity *= 2;
            continue;
        }
        path->len = required_len;
        return true;
    }
#else
    if (rz_getenv("TMPDIR", path)) return true;
#    if RZ_TARGET_OS_ANDROID
    rz_str_append_sv(path, rz_sv_static("/data/local/tmp"));
#    elif RZ_TARGET_FAMILY_APPLE
    rz_usize len = confstr(_CS_DARWIN_USER_TEMP_DIR, rz__path_cstr_buf, RZ__PATH_CSTR_BUF_LEN);
    // not found or error
    if (len == 0) {
        rz_str_append_sv(path, rz_sv_static("/tmp"));
        return true;
    }
    rz_str_append_sized_str(path, rz__path_cstr_buf, len - 1);
#    else
    rz_str_append_sv(path, rz_sv_static("/tmp"));
#    endif
    return true;
#endif
}

static bool rz__fs_symlink_soft(const char *orig, const char *link, bool orig_is_dir) {
#if RZ_TARGET_OS_WINDOWS
    auto flags = (orig_is_dir) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
    if (CreateSymbolicLinkA(link, orig, flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) return true;
    if (rz_last_os_error() == ERROR_INVALID_PARAMETER) {
        // Older Windows objects to SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE,
        // so if we encounter ERROR_INVALID_PARAMETER, retry without that flag.
        if (CreateSymbolicLinkA(link, orig, flags)) return true;
    }
#else
    RZ_UNUSED(orig_is_dir);
    if (symlink(orig, link) == 0) return true;
#endif
    RZ_OS_ERROR_INTR("Failed to create symlink for (%s => %s)", orig, link);
    return false;
}
static bool rz__fs_symlink_hard(const char *orig, const char *linkp) {
#if RZ_TARGET_OS_WINDOWS
    if (CreateHardLinkA(linkp, orig, NULL)) return true;
#else
    // Android has `linkat` on newer versions, but we happen to know
    // `link` always has the correct behavior, so it's here as well.
#    if RZ_TARGET_OS_ANDROID
    if (link(orig, linkp) == 0) return true;
#    else
    if (linkat(AT_FDCWD, orig, AT_FDCWD, linkp, 0) == 0) return true;
#    endif
#endif
    RZ_OS_ERROR_INTR("Failed to create hardlink for (%s => %s)", orig, linkp);
    return false;
}

static bool rz__fs_copy_file(const char *src_path, const char *dst_path) {
#if RZ_TARGET_OS_WINDOWS
    rz_u64 size = 0;
    if (CopyFileA(src_path, dst_path, TRUE)) return true;
    RZ_OS_ERROR_INTR("Failed to copy file (%s => %s)", orig, linkp);
    return false;
#else
    bool  result = true;
    RZ_Fd src_fd = RZ_INVALID_FD;
    RZ_Fd dst_fd = RZ_INVALID_FD;
    src_fd       = rz_fd_open_cstr_read(src_path);
    if (src_fd == RZ_INVALID_FD) rz_return_defer(false);

    struct stat src_stat = {0};
    if (fstat(src_fd, &src_stat) != 0) {
        RZ_OS_ERROR_INTR("Failed to get metadata from fd: %d", src_fd);
        rz_return_defer(false);
    }
    dst_fd = rz_fd_open_cstr_write(dst_path, .perm = src_stat.st_mode);
    if (dst_fd == RZ_INVALID_FD) rz_return_defer(false);

#    if RZ_TARGET_ANY(OS, LINUX, ANDROID)
    off_t copied = 0;
    while (copied < src_stat.st_size) {
        ssize_t written = sendfile(src_fd, dst_fd, &copied, SSIZE_MAX);
        if (written < 0) rz_return_defer(false);
        copied += written;
    }
#    elif RZ_TARGET_FAMILY_APPLE
    // fcopyfile(3) is supported on OS X 10.5+
    if (fcopyfile(src_fd, dst_fd, 0, COPYFILE_ALL) < 0) rz_return_defer(false);

#    elif RZ_TARGET_OS_FREEBSD
    off_t copied = 0;
    while (copied < file_stat.st_size) {
        ssize_t written = copy_file_range(srz_fd, 0, dst_fd, NULL, SSIZE_MAX, 0);
        if (written == -1) rz_return_defer(false);
        copied += written;
    }
#    else
    RZ_BytesArray array = {.allocator = rz_std_allocator()};
    if (rz_fd_read_all(src_fd, &array)) {
        if (!rz_fd_write_all(dst_fd, array.data, array.len)) {
            result = false;
        }
    } else {
        result = false;
    }
    rz_arr_free(&array);
#    endif
defer:
    if (!result) RZ_OS_ERROR_INTR("Failed to copy file: (%s => %s)", src_path, dst_path);
    if (src_fd != RZ_INVALID_FD) rz_fd_close(src_fd);
    if (dst_fd != RZ_INVALID_FD) rz_fd_close(dst_fd);
    return result;

#endif
    RZ_TODO("rz__fs_copy_file");
}

RZ_DEF bool rz_fs_copy_opt(const RZ_Path src_path, const RZ_Path dst_path, RZ_FsCopyOpt opt) {
    bool result                   = true;

    RZ_PathMetadata  src_meta     = {0};
    RZ_PathMetadata  dst_meta     = {0};
    RZ_PathMetadata *dst_meta_ref = NULL;

    RZ_Walkdir   *wd              = NULL;
    RZ_FsDirEntry entry           = {0};

    RZ_PathBuf src_path_buf       = {.allocator = rz_std_allocator()};
    RZ_PathBuf dst_path_buf       = {.allocator = rz_std_allocator()};

    // if its failed. its mean error or the src_path is not exists
    if (!rz_path_symlink_metadata(src_path, &src_meta)) rz_return_defer(false);
    // RZ_Path src_path = src_path;
    if (rz_filetype_is_symlink(src_meta.type)) {
        switch (opt.symlink) {
        case RZ_FS_COPY_SYMLINK_FOLLOW:
            // get the metadata for path symlink is pointed to
            if (!rz_path_metadata(src_path, &src_meta)) rz_return_defer(false);
            // follow symlink and get path symlink pointed to
            if (!rz_fs_read_symlink(src_path, &src_path_buf)) rz_return_defer(false);
            break;
        case RZ_FS_COPY_SYMLINK_SKIP:
            RZ_LOGI_INTR("Skipped copy because (src_path: " RZ_SVFmt ") is symlink and opt.symlink set to RZ_FS_COPY_SYMLINK_SKIP", RZ_SVArg(src_path));
            rz_return_defer(true);
            break;
        case RZ_FS_COPY_SYMLINK_COPY:
            break;
        default:
            RZ_UNREACHABLE("switch: opt.symlink");
            break;
        }
    }
    bool is_src_dir_recursive = rz_filetype_is_dir(src_meta.type) && opt.recursive;
    if (!rz_filetype_is_dir(src_meta.type) && opt.directories_only) {
        RZ_ERROR_INTR("Try to copy (path: " RZ_SVFmt "). with opt.directories_only is set. but the path is not directory", RZ_SVArg(src_path));
        rz_return_defer(false);
    }

    if (rz_path_metadata(dst_path, &dst_meta)) {
        dst_meta_ref                       = &dst_meta;
        bool is_dst_file_src_dir_recursive = !rz_filetype_is_dir(dst_meta.type) && !is_src_dir_recursive;

        switch (opt.exists) {
        case RZ_FS_COPY_EXISTS_ERROR: {
            if (is_dst_file_src_dir_recursive) {
                RZ_ERROR_INTR("Try to copy (path: " RZ_SVFmt ") to existing (path: " RZ_SVFmt ")", RZ_SVArg(src_path), RZ_SVArg(dst_path));
                rz_return_defer(false);
            }
        } break;
        case RZ_FS_COPY_EXISTS_SKIP: {
            if (is_dst_file_src_dir_recursive) {
                RZ_LOGI_INTR("Skipped copy because (src_path: " RZ_SVFmt ") to (dst_path: " RZ_SVFmt
                             "). but dst_path is already exists. but (opt.exists == RZ_FS_COPY_EXISTS_SKIP).",
                             RZ_SVArg(src_path), RZ_SVArg(dst_path));
                rz_return_defer(true);
            }
        } break;
        case RZ_FS_COPY_EXISTS_OVERWRITE: {
            /// TODO: implement recoverable way if failed after remove / overwrite the dst_path
            if (!rz_fs_remove(dst_path, .all = true)) rz_return_defer(false);
        } break;
        default:
            RZ_UNREACHABLE("opt.exists: %d", opt.exists);
            break;
        }
    }

    // src_path is not directory and opt.recursive is not set
    if (!is_src_dir_recursive) {
        switch (opt.mode) {
        case RZ_FS_COPY_CONTENT:
            // result = rz__fs_copy_file(src_path, src_meta, dst_path, dst_meta_ref);
            break;
        case RZ_FS_COPY_SOFT_LINK:
            // result = rz__fs_symlink_soft(src_path, src_meta, dst_path, dst_meta_ref);
            break;
        case RZ_FS_COPY_HARD_LINK:
            // result = rz__fs_symlink_hard(src_path, src_meta, dst_path, dst_meta_ref);
            break;
        default:
            RZ_UNREACHABLE("opt.mode: %d", opt.mode);
            break;
        }
        goto defer;
    }
    if ((opt.mode == RZ_FS_COPY_SOFT_LINK || opt.mode == RZ_FS_COPY_HARD_LINK) && is_src_dir_recursive) {
        RZ_UNIMPLEMENTED("impl this");
    }

    wd = rz_walkdir_open(src_path, .allocator = rz_std_allocator(), .recursive = opt.recursive, .hidden = !opt.exclude_hidden);
    if (wd == NULL) rz_return_defer(false);

    rz_walkdir_foreach(wd, &entry, status) {
        if (status == RZ_DIR_ENTRY_ERROR) rz_return_defer(false);
        if (rz_filetype_is_dir(entry.type)) {
            continue;
        }
        switch (opt.mode) {
        case RZ_FS_COPY_CONTENT:
            // result = rz__fs_copy_file(src_path, src_meta, dst_path, dst_meta_ref);
            break;
        case RZ_FS_COPY_SOFT_LINK:
            // result = rz__fs_symlink_soft(src_path, src_meta, dst_path, dst_meta_ref);
            break;
        case RZ_FS_COPY_HARD_LINK:
            // result = rz__fs_symlink_hard(src_path, src_meta, dst_path, dst_meta_ref);
            break;
        default:
            RZ_UNREACHABLE("opt.mode: %d", opt.mode);
            break;
        }
    }

defer:
    if (wd) rz_walkdir_close(wd);
    rz_arr_free(&src_path_buf);
    rz_arr_free(&dst_path_buf);
    return result;
}

typedef enum : rz_u8
{
    RZ__CREATE_DIR_STATUS_OK = 0,
    RZ__CREATE_DIR_STATUS_ALREADY_EXISTS,
    RZ__CREATE_DIR_STATUS_PARENT_NOT_EXISTS,
    RZ__CREATE_DIR_STATUS_ERROR,
} RZ__CreateDirStatus;
static RZ__CreateDirStatus rz__fs_create_dir(const RZ_Path dir_path, rz_u32 perm) {
    RZ__CreateDirStatus result    = RZ__CREATE_DIR_STATUS_OK;
    auto                m         = rz_temp_snapshot();
    auto                path_cstr = rz__path_cstr_temp(dir_path);
#if RZ_TARGET_OS_WINDOWS
    if (!CreateDirectoryA(path_cstr, NULL)) {
        auto errc = rz_last_os_error();
        if (errc == ERROR_ALREADY_EXISTS) rz_return_defer(RZ__CREATE_DIR_STATUS_ALREADY_EXISTS);
        if (errc == ERROR_PATH_NOT_FOUND) rz_return_defer(RZ__CREATE_DIR_STATUS_PARENT_NOT_EXISTS);
        rz_return_defer(RZ__CREATE_DIR_STATUS_ERROR);
    }
#else
    if (mkdir(path_cstr, perm) != 0) {
        auto errc = rz_last_os_error();
        if (errc == EEXIST) rz_return_defer(RZ__CREATE_DIR_STATUS_ALREADY_EXISTS);
        if (errc == ENOENT) rz_return_defer(RZ__CREATE_DIR_STATUS_PARENT_NOT_EXISTS);
        rz_return_defer(RZ__CREATE_DIR_STATUS_ERROR);
    }
#endif
defer:
    rz_temp_rewind(m);
    return result;
}

RZ_DEF bool rz_fs_create_dir_opt(const RZ_Path dir_path, RZ_FsCreateDirOpt opt) {
    if (rz_path_is_empty(dir_path)) return true;
    if (opt.perm == 0) opt.perm = RZ_DEFAULT_DIRECTORY_CREATION_PERMISION;

    bool result                     = true;
    RZ_Array(RZ_Path) uncreated_dir = {.allocator = rz_std_allocator()};

    auto ret                        = rz__fs_create_dir(dir_path, opt.perm);
    switch (ret) {
    case RZ__CREATE_DIR_STATUS_OK:
        rz_return_defer(true);
    case RZ__CREATE_DIR_STATUS_ALREADY_EXISTS: {
        if (opt.skip_exists) {
            RZ_LOGI_INTR("Create Directory: " RZ_PathFmt ". is already exists.", RZ_PathArg(dir_path));
            rz_return_defer(true);
        }
        rz_return_defer(false);
    }
    case RZ__CREATE_DIR_STATUS_PARENT_NOT_EXISTS: {
        if (opt.all) {
            rz_arr_append(&uncreated_dir, dir_path);
            break;
        }
        // fallthrough
    }
    case RZ__CREATE_DIR_STATUS_ERROR:
        RZ_OS_ERROR_INTR("Failed to create directory: " RZ_PathFmt, RZ_PathArg(dir_path))
        rz_return_defer(false);
    default:
        RZ_UNREACHABLE("rz__fs_create_dir");
    }

    RZ_Path dir        = dir_path;
    RZ_Path dir_parent = {0};
    while (rz_path_parent_dir(dir, &dir_parent)) {
        dir             = dir_parent;

        bool break_loop = false;

        auto ret        = rz__fs_create_dir(dir_parent, opt.perm);
        switch (ret) {
        case RZ__CREATE_DIR_STATUS_OK:
            break_loop = true;
            break;
        case RZ__CREATE_DIR_STATUS_PARENT_NOT_EXISTS:
            rz_arr_append(&uncreated_dir, dir_parent);
            break;

        case RZ__CREATE_DIR_STATUS_ALREADY_EXISTS: {
            if (rz_path_is_dir(dir_parent)) {
                break_loop = true;
                break;
            }
            // if its already exists but not directory. error
            // fallthrough
        }
        case RZ__CREATE_DIR_STATUS_ERROR:
            RZ_OS_ERROR_INTR("Failed to create parent directory " RZ_PathFmt ". while creating directory " RZ_PathFmt, RZ_PathArg(dir_parent), RZ_PathArg(dir_path))
            rz_return_defer(false);
        default:
            RZ_UNREACHABLE("rz__fs_create_dir");
        }

        if (break_loop) break;
    }

    for (rz_usize i = uncreated_dir.len; i-- > 0;) {
        dir      = uncreated_dir.data[i];
        auto ret = rz__fs_create_dir(dir, opt.perm);
        if (ret != RZ__CREATE_DIR_STATUS_OK) {
            if ((ret == RZ__CREATE_DIR_STATUS_ALREADY_EXISTS) && !rz_path_is_dir(dir)) {
                RZ_OS_ERROR_INTR("Failed to create parent directory " RZ_PathFmt ". while creating directory " RZ_PathFmt, RZ_PathArg(dir_parent), RZ_PathArg(dir))
                rz_return_defer(false);
            }
        }
    }
defer:
    rz_arr_free(&uncreated_dir);
    return result;
}

static bool rz__fs_remove_file(const RZ_Path path, bool ignore_non_existing, bool ignore_access_denied) {
    bool result    = true;
    auto m         = rz_temp_snapshot();
    auto path_cstr = rz__path_cstr_temp(path);
#if RZ_TARGET_OS_WINDOWS
    if (DeleteFileA(path_cstr)) rz_return_defer(true);

    auto last_err = rz_last_os_error();
    if ((last_err == ERROR_FILE_NOT_FOUND) && ignore_non_existing) {
        RZ_LOGI_INTR("Try to remove non existing (file: %s). but ignored", path_cstr);
        rz_return_defer(true);
    }
    if (last_err == ERROR_ACCESS_DENIED) {
        if (ignore_access_denied) {
            RZ_LOGI_INTR("Try to remove access denied (file: %s). but ignored", path_cstr);
            rz_return_defer(true);
        }
        // TODO: `DeleteFileW` fails with ERROR_ACCESS_DENIED then try to remove the file while ignoring the readonly attribute.
    }
    RZ_OS_ERROR_INTR("Failed to remove (file: %s)", path_cstr);
    result = false;
#else
    if (unlink(path_cstr) == 0) rz_return_defer(true);

    auto last_err = rz_last_os_error();
    if ((last_err == ENOENT) && ignore_non_existing) {
        RZ_LOGI_INTR("Try to remove non existing (file: %s). but ignored", path_cstr);
        rz_return_defer(true);
    }
    if (((last_err == EPERM) || (last_err == EACCES)) && ignore_access_denied) {
        RZ_LOGI_INTR("Try to remove access denied (file: %s). but ignored", path_cstr);
        rz_return_defer(true);
    }
    RZ_OS_ERROR_INTR("Failed to remove (file: %s)", path_cstr);
    result = false;
#endif
defer:
    rz_temp_rewind(m);
    return result;
}
static bool rz__fs_remove_dir(const RZ_Path path, bool ignore_non_existing, bool ignore_access_denied) {
    bool result    = true;
    auto m         = rz_temp_snapshot();
    auto path_cstr = rz__path_cstr_temp(path);
#if RZ_TARGET_OS_WINDOWS
    if (RemoveDirectoryA(path_cstr)) rz_return_defer(true);

    auto last_err = rz_last_os_error();
    if ((last_err == ERROR_FILE_NOT_FOUND) && ignore_non_existing) {
        RZ_LOGI_INTR("Try to remove non existing (directory: %s). but ignored", path_cstr);
        rz_return_defer(true);
    } else if ((last_err == ERROR_ACCESS_DENIED) && ignore_access_denied) {
        RZ_LOGI_INTR("Try to remove access denied (directory: %s). but ignored", path_cstr);
        rz_return_defer(true);
    }
    RZ_OS_ERROR_INTR("Failed to remove (directory: %s)", path_cstr);
    result = false;
#else
    if (rmdir(path_cstr) == 0) rz_return_defer(true);

    auto last_err = rz_last_os_error();
    if ((last_err == ENOENT) && ignore_non_existing) {
        RZ_LOGI_INTR("Try to remove non existing (directory: %s). but ignored", path_cstr);
        rz_return_defer(true);
    } else if (((last_err == EPERM) || (last_err == EACCES)) && ignore_access_denied) {
        RZ_LOGI_INTR("Try to remove access denied (directory: %s). but ignored", path_cstr);
        rz_return_defer(true);
    }
    RZ_OS_ERROR_INTR("Failed to remove (directory: %s)", path_cstr);
    result = false;
#endif
defer:
    rz_temp_rewind(m);
    return result;
}

RZ_DEF bool rz_fs_remove_opt(const RZ_Path path, RZ_FsRemoveOpt opt) {
    if (opt.remove_to_trash) {
        RZ_UNIMPLEMENTED("remove_to_trash");
        return false;
    }

    bool            result   = true;
    RZ_PathMetadata metadata = {0};
    RZ_Walkdir     *wd       = NULL;

    if (!rz_path_symlink_metadata(path, &metadata)) rz_return_defer(false);
    if (!rz_filetype_is_dir(metadata.type)) {
        rz_return_defer(rz__fs_remove_file(path, opt.ignore_non_existing, opt.ignore_access_denied));
    } else if (!opt.all) {
        rz_return_defer(rz__fs_remove_dir(path, opt.ignore_non_existing, opt.ignore_access_denied));
    }

    wd = rz_walkdir_open(path, .allocator = rz_std_allocator(), .entry_file_first = true, .ignore_error = true, .recursive = true, .hidden = true);
    if (wd == NULL) rz_return_defer(false);

    RZ_FsDirEntry entry = {0};
    rz_walkdir_foreach(wd, &entry, status) {
        if (status == RZ_DIR_ENTRY_ERROR) {
            if (opt.ignore_access_denied) continue;
            break;
        }

        if (rz_filetype_is_dir(entry.type)) {
            result = rz__fs_remove_dir(entry.path, opt.ignore_non_existing, opt.ignore_access_denied);
        } else {
            result = rz__fs_remove_file(entry.path, opt.ignore_non_existing, opt.ignore_access_denied);
        }
        if (!result) break;
    }

defer:
    if (wd) rz_walkdir_close(wd);
    return result;
}

RZ_DEF bool rz_fs_rename_opt(const RZ_Path old, const RZ_Path new, RZ_FsRenameOpt opt) {
    bool result = true;
    auto m      = rz_temp_snapshot();

    if (!rz_fs_exists(old)) {
        RZ_LOGI_INTR("Try to Rename non existing path (" RZ_PathFmt " => " RZ_PathFmt ")", RZ_PathArg(old), RZ_PathArg(new));
        return false;
    }

    bool new_exists = rz_fs_exists(new);
    if (new_exists && !opt.replace_existing) {
        if (opt.ignore_existing) {
            RZ_LOGI_INTR("Rename to existing path (" RZ_PathFmt " => " RZ_PathFmt ")", RZ_PathArg(old), RZ_PathArg(new));
            return true;
        }
        RZ_ERROR_INTR("Failed to Rename existing path (" RZ_PathFmt " => " RZ_PathFmt "). path new already exists. try to set .replace_existing to replace the existing path",
                      RZ_PathArg(old), RZ_PathArg(new));
        return false;
    }

    auto old_cstr = rz__path_cstr_temp(old);
    auto new_cstr = rz__path_cstr_temp(new);
#if RZ_TARGET_OS_WINDOWS
    if (MoveFileExA(old_cstr, new_cstr, (opt.replace_existing) ? MOVEFILE_REPLACE_EXISTING : 0)) rz_return_defer(true);
    if (rz_last_os_error() == ERROR_ACCESS_DENIED) {
        // TODO:
        // if `MoveFileExW` fails with ERROR_ACCESS_DENIED then try to move
        // the file while ignoring the readonly attribute.
        // This is accomplished by calling `SetFileInformationByHandle` with `FileRenameInfoEx`.
    }
    RZ_OS_ERROR_INTR("Failed to rename path ('%s' => '%s')", old_cstr, new_cstr);
    result = false;
#else
    if (rename(old_cstr, new_cstr) == 0) rz_return_defer(true);
    RZ_OS_ERROR_INTR("Failed to rename path ('%s' => '%s')", old_cstr, new_cstr);
    result = false;
#endif
defer:
    rz_temp_rewind(m);
    return result;
}

RZ_DEF bool rz_fs_read(const RZ_Path path, RZ_BytesArray *bytes) {
    RZ_Fd fd = rz_fd_open_read(path);
    if (fd == RZ_INVALID_FD) return false;

    bool result = rz_fd_read_all(fd, bytes);

    rz_fd_close(fd);
    return result;
}

RZ_DEF bool rz_fs_read_to_string(const RZ_Path path, RZ_Str *string) {
    return rz_fs_read(path, (RZ_BytesArray *)string);
}

RZ_DEF bool rz_fs_write(const RZ_Path path, const rz_u8 *bytes, rz_usize bytes_len) {
    RZ_Fd fd = rz_fd_open_write(path);
    if (fd == RZ_INVALID_FD) return false;
    bool result = rz_fd_write_all(fd, bytes, bytes_len);
    rz_fd_close(fd);
    return result;
}

RZ_DEF RZ_PathModifiedStatus rz_fs_is_modified(const RZ_Path input, const RZ_Path output) {
    RZ_PathMetadata m_input  = {0};
    RZ_PathMetadata m_output = {0};
    if (!rz_path_metadata(input, &m_input)) return RZ_PATH_MODIF_ERROR;
    if (!rz_path_metadata(output, &m_output)) return RZ_PATH_MODIF_ERROR;

    // if time modified of input is greater than outout. its mean the input is modified
    if (rz_duration_cmp(m_input.modified.t, m_output.modified.t) > 0) {
        return RZ_PATH_MODIF_CHANGED;
    } else {
        return RZ_PATH_MODIF_UNCHANGED;
    }
}

RZ_DEF RZ_Fd rz_fd_open_opt(const RZ_Path path, RZ_FdOpenOpt o) {
    RZ_Fd       result    = RZ_INVALID_FD;
    auto        m         = rz_temp_snapshot();
    const char *path_cstr = rz__path_cstr_temp(path);
#if RZ_TARGET_OS_WINDOWS
    /// FIXME: the windows implementation is suck and i think is wrong. fix later.
    DWORD desiredAccess       = 0;
    DWORD creationDisposition = 0;
    DWORD flagsAndAttributes  = FILE_ATTRIBUTE_NORMAL;
    DWORD shareMode           = RZ__FILE_SHARE_MODE_DEFAULT; // allow others to read/write unless you want exclusive

    if (o.read) desiredAccess |= GENERIC_READ;
    if (o.write) desiredAccess |= GENERIC_WRITE;

    if (o.create && o.truncate) {
        creationDisposition = CREATE_ALWAYS;     // create or overwrite
    } else if (o.create) {
        creationDisposition = OPEN_ALWAYS;       // open if exists, else create
    } else if (o.truncate) {
        creationDisposition = TRUNCATE_EXISTING; // must exist
    } else {
        creationDisposition = OPEN_EXISTING;
    }

    if (o.append) {
        // For append semantics, still open with write access; callers should SetFilePointer to end before writes.
        // Keep creationDisposition as chosen above.
        // No special flag needed here, but ensure GENERIC_WRITE is present.
        desiredAccess |= GENERIC_WRITE;
    }

    if (!o.read && o.write) {
        // if write-only, allow read-sharing by others; keep attributes normal
    } else if (o.read && !o.write) {
        // read-only open
        flagsAndAttributes = FILE_ATTRIBUTE_READONLY;
    }

    SECURITY_ATTRIBUTES saAttr  = {0};
    saAttr.nLength              = sizeof(saAttr);
    saAttr.bInheritHandle       = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    result                      = CreateFileA(path_cstr, desiredAccess, shareMode, &saAttr, creationDisposition, flagsAndAttributes, NULL);
    if (result == INVALID_HANDLE_VALUE) {
        // map Win32 error to your rz error handling if desired
        rz_return_defer(RZ_INVALID_FD);
    }
    // If opened with append semantics, move pointer to end.
    if (o.append) {
        SetFilePointer(result, 0, NULL, FILE_END);
    }
#else
    // set read or write or read and write
    rz_int flag = 0;
    if ((o.read == true) && (o.write == true)) rz_bit_set(flag, O_RDWR);
    else {
        if (o.read) rz_bit_set(flag, O_RDONLY);
        if (o.write) rz_bit_set(flag, O_WRONLY);
    }

    // if .append or .truncate auto add write.
    if (o.append == true) rz_bit_set_all(flag, O_APPEND, O_WRONLY);
    if (o.truncate == true) rz_bit_set_all(flag, O_TRUNC, O_WRONLY);

    // determine if is creating new file.
    if ((o.create_new == true) || (o.create == true)) {
        if ((o.write == false) || (o.append == false)) {
            errno = EINVAL; // invalid argument
            RZ_OS_ERROR_INTR("Failed to open file opening file (path: '" RZ_SVFmt "'). .create set without .write or .append", RZ_SVArg(path));
            rz_return_defer(RZ_INVALID_FD);
        }
        if (o.perm == 0) o.perm = RZ_DEFAULT_OPEN_CREATION_PERMISION;

        rz_bit_set(flag, O_CREAT); // set O_CREAT bit
        if (o.create_new) {
            if (rz_fs_exists(path)) {
                errno = EEXIST;    // file already exists
                RZ_OS_ERROR_INTR("Failed to open file (path: " RZ_SVFmt ") with options .create_new file.", RZ_SVArg(path));
                rz_return_defer(RZ_INVALID_FD);
            }
            rz_bit_clear(flag, O_TRUNC); // clear the truncate bit
        }
        result = open(path_cstr, flag, (mode_t)o.perm);
    } else {
        // if .create_new  or .create not set. open without perm
        result = open(path_cstr, flag);
    }

    if (result < 0) {
        RZ_OS_ERROR_INTR("opening file: '" RZ_SVFmt "'.create set without .write or .append", RZ_SVArg(path));
        result = RZ_INVALID_FD;
    }
#endif
defer:
    rz_temp_rewind(m);
    return result;
}

RZ_DEF bool rz_fd_close(RZ_Fd fd) {
#if RZ_TARGET_OS_WINDOWS
    return CloseHandle(fd);
#else
    return close(fd) != 0;
#endif // _WIN32
}

RZ_DEF bool rz_fd_read(RZ_Fd fd, void *dst, rz_usize dstsize, rz_usize *readsize) {
    if (dst == NULL || readsize == NULL) {
        errno = EINVAL; // invalid argument
        RZ_OS_ERROR_INTR("argument dst or result argument readsize should be not null");
        return false;
    }
#if RZ_TARGET_OS_WINDOWS
    if (!ReadFile(fd, dst, dstsize, (LPDWORD)readsize, NULL)) {
        RZ_OS_ERROR_INTR("Failed to read file.");
        return false;
    }
    return true;
#else
    *readsize = read(fd, dst, dstsize);
    if (*readsize < 0) {
        RZ_OS_ERROR_INTR("Failed to read file.");
        return false;
    }
    return true;
#endif
}

RZ_DEF bool rz_fd_write(RZ_Fd fd, const void *src, rz_usize srclen, rz_usize *writesize) {
    if (src == NULL || writesize == NULL) {
        errno = EINVAL; // invalid argument
        RZ_OS_ERROR_INTR("argument src or result argument readsize should be not null");
        return false;
    }
#if RZ_TARGET_OS_WINDOWS
    if (!WriteFile(fd, src, srclen, (LPDWORD)writesize, NULL)) {
        RZ_OS_ERROR_INTR("Failed to write file.");
        return false;
    }
    return true;
#else
    *writesize = write(fd, src, srclen);
    if (*writesize < 0) {
        RZ_OS_ERROR_INTR("Failed to read file.");
        return false;
    }
    return true;
#endif
}

RZ_DEF bool rz_fd_read_all(RZ_Fd fd, RZ_BytesArray *bytes) {
    RZ_DBG_ASSERT_NOT_NULL(bytes);
    RZ_PathMetadata m = {0};
    if (!rz__path_metadata_from_fd(fd, &m)) return false;

    rz_arr_reserve(bytes, m.size);
    while (bytes->len < m.size) {
        rz_usize readed = 0;
        if (!rz_fd_read(fd, bytes->data, bytes->capacity, &readed)) return false;
        if (readed == 0) break;
        bytes->len += readed;
    }

    return true;
}

RZ_DEF bool rz_fd_write_all(RZ_Fd fd, const rz_u8 *src, rz_usize srclen) {
    if (src == NULL) {
        errno = EINVAL; // invalid argument
        RZ_OS_ERROR_INTR("argument src or result argument readsize should be not null");
        return false;
    }
    const rz_u8 *s        = src;
    rz_usize     writelen = 0;
    while (srclen > 0) {
        if (!rz_fd_write(fd, s, srclen, &writelen)) return false;

        s += writelen;
        srclen -= writelen;
    }
    return true;
}

RZ_DEF RZ_Fd rz_fd_stdin(void) {
#if RZ_TARGET_OS_WINDOWS
    return GetStdHandle(STD_INPUT_HANDLE);
#else
    return STDIN_FILENO;
#endif // _WIN32
}

RZ_DEF RZ_Fd rz_fd_stdout(void) {
#if RZ_TARGET_OS_WINDOWS
    return GetStdHandle(STD_OUTPUT_HANDLE);
#else
    return STDOUT_FILENO;
#endif // _WIN32
}

RZ_DEF RZ_Fd rz_fd_stderr(void) {
#if RZ_TARGET_OS_WINDOWS
    return GetStdHandle(STD_ERROR_HANDLE);
#else
    return STDERR_FILENO;
#endif // _WIN32
}

RZ_DEF bool rz_fd_pipe(RZ_FdPipe *pp) {
    RZ_DBG_ASSERT_NOT_NULL(pp);
#if RZ_TARGET_OS_WINDOWS
    // https://docs.microsoft.com/en-us/windows/win32/ProcThread/creating-a-child-process-with-redirected-input-and-output
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength             = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle      = TRUE;

    if (!CreatePipe(&pp->read, &pp->write, &saAttr, 0)) {
        RZ_OS_ERROR_INTR("Failed to create pipe");
        return false;
    }

    return true;
#else
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        RZ_OS_ERROR_INTR("Failed to create pipe");
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
#if RZ_TARGET_OS_WINDOWS
    if (res->capacity == 0) {
        rz_arr_reserve(res, 512);
    }
    DWORD ret_size = 0;
    while (true) {
        if ((ret_size = GetEnvironmentVariableA(name, res->data, res->capacity)) == 0) {
            RZ_OS_ERROR_INTR("failed to get environment variable");
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
    if (v == NULL) return false;
    rz_str_append_cstr(res, v);
#endif
    return true;
}

struct RZ_FsDir {
    RZ_PathBuf entrybuf;
    rz_usize   path_len;

#if RZ_TARGET_OS_WINDOWS
    WIN32_FIND_DATA win32_data;
    HANDLE          win32_hFind;
    bool            win32_init;
#else
    DIR           *posix_dir;
    struct dirent *posix_ent;
#endif

    // only used by walkdir
    RZ_PathFileType __type;
};

RZ_DEF RZ_FsDir *rz_dir_open(const RZ_Path path, RZ_Allocator a) {
    bool      result = true;
    RZ_FsDir *res    = rz_calloc(a, res, 1);
    RZ_ASSERT_ALLOCATOR_PTR(res);
    res->path_len = path.len;
    res->entrybuf = rz_pathbuf_from_path_alloc(path, a);
    auto m        = rz_temp_snapshot();
#if RZ_TARGET_OS_WINDOWS
    char *cstr       = rz_tsprintf(RZ_SVFmt "\\*", RZ_SVArg(path));
    res->win32_hFind = FindFirstFileA(cstr, &res->win32_data);
    if (res->win32_hFind == INVALID_HANDLE_VALUE) rz_return_defer(false);
#else
    res->posix_dir = opendir(rz__path_cstr_temp(path));
    if (res->posix_dir == NULL) rz_return_defer(false);
#endif
defer:
    if (!result) {
        RZ_OS_ERROR_INTR("Failed to open directory '" RZ_SVFmt "'", RZ_SVArg(path));
        rz_free(a, res, 1);
        res = NULL;
    }
    rz_temp_rewind(m);
    return res;
}

RZ_DEF void rz_dir_close(RZ_FsDir *dir) {
    if (dir == NULL) return;
    RZ_Allocator a = dir->entrybuf.allocator;
#if RZ_TARGET_OS_WINDOWS
    FindCloseA(dir->win32_hFind);
#else
    if (dir->posix_dir) closedir(dir->posix_dir);
#endif
    rz_pathbuf_free(&dir->entrybuf);
    rz_free(a, dir, 1);
}

RZ_DEF RZ_FsDirEntryStatus rz_dir_next(RZ_FsDir *dir, RZ_FsDirEntry *entry) {
    RZ_DBG_ASSERT_NOT_NULL(dir);
    RZ_StrView filename = {0};
    dir->entrybuf.len   = dir->path_len;
#if RZ_TARGET_OS_WINDOWS
    if (!dir->win32_init) {
        dir->win32_init = true;
        filename        = rz_sv(dir->win32_data.cFileName);
    } else {
        if (!FindNextFileA(dir->win32_hFind, &dir->win32_data)) {
            if (rz_last_os_error() == ERROR_NO_MORE_FILES) return false;
            RZ_OS_ERROR_INTR("Failed read next directory entry");
            entry->path = rz_path_empty;
            return false;
        }
        filename         = rz_sv(dir->win32_data.cFileName);
        auto reparse_tag = (rz_bit(dir->win32_data.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT) ? dir->win32_data.dwReserved0 : 0);
        entry->type      = rz__path_filetype(dir->win32_data.dwFileAttributes, reparse_tag);
    }
#else
    for (;;) {
        errno          = 0;
        dir->posix_ent = readdir(dir->posix_dir);
        if (dir->posix_ent == NULL) {
            if (rz_last_os_error() == 0) return false;
            RZ_OS_ERROR_INTR("Failed to read next directory entry");
            entry->path = rz_path_empty;
            return false;
        }
        filename = rz_sv(dir->posix_ent->d_name);
        if (!(rz_sv_eq(filename, rz_sv_static(".")) || rz_sv_eq(filename, rz_sv_static("..")))) break;
    }

    if (rz_sv_starts_with_char(filename, '.')) rz_bit_set(entry->type, RZ_PATH_FILE_TYPE_HIDDEN);
    rz_bit_set(entry->type, rz__path_filetype(DTTOIF(dir->posix_ent->d_type)));

#endif
    rz_pathbuf_push(&dir->entrybuf, filename);
    entry->path = rz_path_from_pathbuf(&dir->entrybuf);
    return true;
}

#define rz__dir_get_path(dir) ((RZ_Path){.data = (dir)->entrybuf.data, .len = (dir)->path_len})

typedef RZ_Array(RZ_FsDir *) RZ__DirStack;
struct RZ_Walkdir {
    RZ__DirStack dir_stack;

    rz_usize depth;
    bool     hidden;
    bool     file_only;
    bool     entry_file_first;
    bool     ignore_error;
};

RZ_DEF RZ_Walkdir *rz_walkdir_open_opt(const RZ_Path path, RZ_WalkdirOpenOpt opt) {
    if (!rz_is_allocator(opt.allocator)) opt.allocator = rz_std_allocator();
    RZ_Walkdir *wd = rz_alloc(opt.allocator, wd, 1);
    RZ_ASSERT_ALLOCATOR_PTR(wd);

    wd->dir_stack.allocator = opt.allocator;
    wd->depth               = (opt.depth) ? opt.depth : ((opt.recursive) ? (rz_usize)-1 : 0);
    wd->hidden              = opt.hidden;
    wd->file_only           = opt.file_only;
    wd->entry_file_first    = opt.entry_file_first;
    wd->ignore_error        = opt.ignore_error;

    RZ_FsDir *dir           = rz_dir_open(path, opt.allocator);
    if (dir == NULL) return NULL;
    rz_arr_append(&wd->dir_stack, dir);
    return wd;
}

RZ_DEF void rz_walkdir_close(RZ_Walkdir *wd) {
    if (wd == NULL) return;
    auto a = wd->dir_stack.allocator;

    rz_foreach(dir, &wd->dir_stack) if (*dir) rz_dir_close(*dir);
    rz_arr_free(&wd->dir_stack);
    rz_free(a, wd, 1);
}

RZ_DEF RZ_FsDirEntryStatus rz_walkdir_next(RZ_Walkdir *wd, RZ_FsDirEntry *entry) {
    RZ_DBG_ASSERT_NOT_NULL(wd);
    RZ_DBG_ASSERT_NOT_NULL(entry);

    RZ_FsDir *dir = NULL;
    for (;;) {
        // finished
        if (wd->dir_stack.len == 0) return RZ_DIR_ENTRY_STOP;

        dir = rz_arr_last(&wd->dir_stack);
        RZ_DBG_ASSERT_NOT_NULL(dir);

        RZ_FsDirEntryStatus res = rz_dir_next(dir, entry);
        switch (res) {
        case RZ_DIR_ENTRY_OK: {
            if (rz_filetype_is_hidden(entry->type) && !wd->hidden) continue;

            if (rz_filetype_is_dir(entry->type) && (wd->depth > wd->dir_stack.len)) {
                if ((dir = rz_dir_open(entry->path, wd->dir_stack.allocator)) == NULL) {
                    // if error just return the path of the dir
                    if (wd->ignore_error) break;
                    return RZ_DIR_ENTRY_ERROR;
                }
                dir->__type = entry->type;
                rz_arr_append(&wd->dir_stack, dir);
                // get file content in the current dir entry
                if (wd->entry_file_first || wd->file_only) continue;
            }
        } break;

        case RZ_DIR_ENTRY_STOP: {
            if (wd->entry_file_first && !wd->file_only) {
                entry->path = rz__dir_get_path(dir);
                entry->type = dir->__type;
                res         = RZ_DIR_ENTRY_OK;
            }
            rz_dir_close(dir);
            wd->dir_stack.len--;
        } break;

        case RZ_DIR_ENTRY_ERROR: {
            if (wd->ignore_error) continue;
            // on unix using readdir is guaranteed per entry error is not exists.
            // the only error is (EBADF Invalid directory stream descriptor dirp).
            // but in implementation always check if dirp is always valid before processing
            // its just windows that has per entry error
        } break;

        default:
            RZ_UNREACHABLE("RZ_FsDirEntryStatus");
            break;
        }
        return res;
    }
}

#undef RZ_TAG
