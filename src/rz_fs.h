#pragma once

#ifndef __RZ_FS_H
#    define __RZ_FS_H
#    include "rz_common.h"
#    include "rz_strings.h"
#    include "rz_time.h"

#    if RZ_TARGET_OS_WINDOWS
typedef HANDLE RZ_Fd;
#        define RZ_INVALID_FD INVALID_HANDLE_VALUE
#        define RZ_PATH_SEP   '\\'
#        define RZ_PATH_ROOT  "C:\\"
#        define RZ_PATH_MAX   MAX_PATH
#    else
typedef int RZ_Fd;
#        define RZ_INVALID_FD -1
#        define RZ_PATH_SEP   '/'
#        define RZ_PATH_ROOT  "/"
#        define RZ_PATH_MAX   PATH_MAX
#    endif
#    define RZ_PATH_WINDOWS_SEP '\\'
#    define RZ_PATH_UNIX_SEP    '/'

#    if RZ_TARGET_FAMILY_UNIX && !RZ_TARGET_COMPILER_WASI
#        ifndef RZ_DEFAULT_OPEN_CREATION_PERMISION
#            // RZ_DEFAULT_OPEN_CREATION_PERMISION: 0644
#            define RZ_DEFAULT_OPEN_CREATION_PERMISION (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#        endif
#        ifndef RZ_DEFAULT_DIRECTORY_CREATION_PERMISION
#            // RZ_DEFAULT_DIRECTORY_CREATION_PERMISION : 0755
#            define RZ_DEFAULT_DIRECTORY_CREATION_PERMISION (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#        endif
#    else
#        ifndef RZ_DEFAULT_OPEN_CREATION_PERMISION
#            define RZ_DEFAULT_OPEN_CREATION_PERMISION 0
#        endif
#        ifndef RZ_DEFAULT_DIRECTORY_CREATION_PERMISION
#            define RZ_DEFAULT_DIRECTORY_CREATION_PERMISION 0
#        endif
#    endif

#    define RZ_PathFmt     "'%.*s'"
#    define RZ_PathArg(sv) ((int)(sv).len), (sv).data

typedef RZ_Str     RZ_PathBuf;
typedef RZ_StrView RZ_Path;

#    define rz_pathbuf_sized(cstr, size)          rz_str_sized_alloc(cstr, size, NULL)
#    define rz_pathbuf_sized_temp(cstr, size)     rz_str_sized_alloc(cstr, size, rz_temp_allocator())

#    define rz_pathbuf_alloc(cstr, alloc)         rz_str_sized_alloc(cstr, rz_strlen(cstr), alloc)
#    define rz_pathbuf(cstr)                      rz_str_sized(cstr, rz_strlen(cstr))
#    define rz_pathbuf_temp(cstr)                 rz_str_sized_temp(cstr, rz_strlen(cstr))

#    define rz_pathbuf_from_path_alloc(sv, alloc) rz_str_sized_alloc((sv).data, (sv).len, alloc)
#    define rz_pathbuf_from_path(sv)              rz_str_sized((sv).data, (sv).len)
#    define rz_pathbuf_from_path_temp(sv)         rz_str_sized_temp((sv).data, (sv).len)

#    define rz_pathbuf_clone(pathbuf)             rz_arr_clone(pathbuf)
#    define rz_pathbuf_clone_temp(pathbuf)        rz_arr_clone_with_allocator(pathbuf, rz_temp_allocator())
#    define rz_pathbuf_view(path)                 ((const RZ_Path *)path)
#    define rz_pathbuf_ref                        rz_pathbuf_view

#    define rz_pathbuf_free                       rz_str_free

RZ_NODISCARD RZ_DEC const char *rz_pathbuf_cstr(RZ_PathBuf *path);

RZ_DEC bool rz_pathbuf_set_filename(RZ_PathBuf *path, RZ_Path filename);
RZ_DEC bool rz_pathbuf_set_filestem(RZ_PathBuf *path, RZ_Path filestem);
RZ_DEC bool rz_pathbuf_set_extension(RZ_PathBuf *path, RZ_Path extension);
RZ_DEC bool rz_pathbuf_add_extension(RZ_PathBuf *path, RZ_Path extension);

RZ_NODISCARD RZ_DEC RZ_PathBuf rz_pathbuf_join(const RZ_PathBuf *path, RZ_Path component);
#    define rz_pathbuf_join_cstr(path, component_cstr) rz_pathbuf_join(path, rz_path(component_cstr))

RZ_DEC void rz_pathbuf_push(RZ_PathBuf *path, RZ_Path component);
#    define rz_pathbuf_push_cstr(path, component_cstr) rz_pathbuf_push(path, rz_path(component_cstr))

#    define rz_path                                    rz_sv
#    define rz_path_sized                              rz_sv_sized
#    define rz_path_from_pathbuf                       rz_sv_from_str
#    define rz_path_empty                              rz_sv_empty
#    define rz_path_static                             rz_sv_static

#    define rz_path_is_empty                           rz_sv_is_empty

RZ_NODISCARD RZ_DEC const char *rz_path_cstr(const RZ_Path path, RZ_Allocator allocator);
#    define rz_path_cstr_temp(path) rz_path_cstr(path, rz_temp_allocator())

RZ_DEC bool rz_path_filename(const RZ_Path path, RZ_Path *filename);
#    define rz_path_filename_cstr(path, filename) rz_path_filename(rz_path(path), filename)

RZ_DEC bool rz_path_filestem(const RZ_Path path, RZ_Path *filestem);
#    define rz_path_filestem_cstr(path, filename) rz_path_filestem(rz_path(path), filename)

RZ_DEC bool rz_path_extension(const RZ_Path path, RZ_Path *extension);
#    define rz_path_extension_cstr(path, extension) rz_path_extension(rz_path(path), extension)

RZ_DEC bool rz_path_has_extension(const RZ_Path path, RZ_Path ext);
#    define rz_path_has_extension_cstr(path, ext) rz_path_has_extension(rz_path(path), ext)

RZ_DEC bool rz_path_parent_dir(const RZ_Path path, RZ_Path *parent_dir);
#    define rz_path_parent_dir_cstr(path, parent_dir) rz_path_has_extension(rz_path(path), parent_dir)

RZ_DEC bool rz_path_is_absolute(const RZ_Path path);
#    define rz_path_is_absolute_cstr(path) rz_path_is_absolute(rz_path(path))

RZ_DEC bool rz_path_is_relative(const RZ_Path path);
#    define rz_path_is_relative_cstr(path) rz_path_is_relative(rz_path(path))

typedef rz_u8 RZ_PathFileType;
enum
{
    RZ_PATH_FILE_TYPE_UNKNOWN = 0,
    RZ_PATH_FILE_TYPE_FILE    = 0b00000001,
    RZ_PATH_FILE_TYPE_DIR     = 0b00000010,
    RZ_PATH_FILE_TYPE_SYMLINK = 0b00000100,
    RZ_PATH_FILE_TYPE_HIDDEN  = 0b00001000,
};

#    define rz_filetype_is_file(filetype)    rz_bit(filetype, RZ_PATH_FILE_TYPE_FILE)
#    define rz_filetype_is_dir(filetype)     rz_bit(filetype, RZ_PATH_FILE_TYPE_DIR)
#    define rz_filetype_is_symlink(filetype) rz_bit(filetype, RZ_PATH_FILE_TYPE_SYMLINK)
#    define rz_filetype_is_hidden(filetype)  rz_bit(filetype, RZ_PATH_FILE_TYPE_HIDDEN)

typedef struct {
    RZ_SystemTime modified;
    RZ_SystemTime accessed;
    RZ_SystemTime created;

    rz_u64          size;
    rz_u32          perm;
    RZ_PathFileType type;
} RZ_PathMetadata;

RZ_DEC bool rz_path_metadata(const RZ_Path path, RZ_PathMetadata *metadata);
#    define rz_path_metadata_cstr(path, metadata) rz_path_metadata(rz_path(path), metadata)

RZ_DEC bool rz_path_symlink_metadata(const RZ_Path path, RZ_PathMetadata *metadata);
#    define rz_path_symlink_metadata_cstr(path, metadata) rz_path_symlink_metadata(rz_path(path), metadata)

RZ_DEC RZ_PathFileType rz_path_filetype(const RZ_Path path);
#    define rz_path_filetype_cstr(path)   rz_path_filetype(rz_path(path))

#    define rz_path_is_file(path)         rz_filetype_is_file(rz_path_filetype(path))
#    define rz_path_is_dir(path)          rz_filetype_is_dir(rz_path_filetype(path))
#    define rz_path_is_symlink(path)      rz_filetype_is_symlink(rz_path_filetype(path))

#    define rz_path_is_file_cstr(path)    rz_filetype_is_file(rz_path_filetype_cstr(path))
#    define rz_path_is_dir_cstr(path)     rz_filetype_is_dir(rz_path_filetype_cstr(path))
#    define rz_path_is_symlink_cstr(path) rz_filetype_is_symlink(rz_path_filetype_cstr(path))

RZ_DEC rz_u64 rz_path_file_size(const RZ_Path path);
#    define rz_path_file_size_cstr(path) rz_path_file_size(rz_path(path))

RZ_DEC RZ_SystemTime rz_path_modified(const RZ_Path path);
#    define rz_path_modified_cstr(path) rz_path_modified(rz_path(path))

RZ_DEC RZ_SystemTime rz_path_accessed(const RZ_Path path);
#    define rz_path_accessed_cstr(path) rz_path_accessed(rz_path(path))

RZ_DEC RZ_SystemTime rz_path_created(const RZ_Path path);
#    define rz_path_created_cstr(path) rz_path_created(rz_path(path))

RZ_DEC bool rz_fs_exists(const RZ_Path path);
#    define rz_fs_exists_cstr(path_cstr) rz_fs_exists(rz_path(path_cstr))

RZ_DEC bool rz_fs_read_symlink(const RZ_Path path, RZ_PathBuf *symlink_result);
#    define rz_fs_read_symlink_cstr(path_cstr, symlink_result) rz_fs_read_symlink(rz_path(path_cstr), symlink_result)

RZ_DEC bool rz_fs_canonicalize(RZ_PathBuf *path);

RZ_DEC bool rz_fs_absolute(RZ_PathBuf *path);
RZ_DEC bool rz_fs_current_dir(RZ_PathBuf *path);
RZ_DEC bool rz_fs_set_current_dir(const RZ_Path path);
RZ_DEC bool rz_fs_current_exe(RZ_PathBuf *path);
RZ_DEC bool rz_fs_home_dir(RZ_PathBuf *path);
RZ_DEC bool rz_fs_temp_dir(RZ_PathBuf *path);

RZ_DEC RZ_StrView rz_fs_env_path(void);
RZ_DEC bool       rz_fs_env_path_next(RZ_StrView *env_path, RZ_Path *path);
#    define rz_env_path_foreach(path) for (RZ_StrView __env_path = rz_fs_env_path(), path = {0}; rz_fs_env_path_next(&__env_path, &path);)

/// Copy Options
/// default behavior:
/// - Report an error if destination is exists.
///     overwrite by (.skip_exists, .overwrite_exists)
/// - Skip directoriues. report if skip_exists or overwrite_exists is set.
///     overwrite by (.recursive)
/// - Follow Symlink.
///     overwrite by (.copy_symlink, .skip_symlink)
/// - Copy file content
///     overwrite by (.directories_only, .create_symlinks, .create_hard_links)
typedef struct {
    enum : rz_u8
    {
        RZ_FS_COPY_EXISTS_ERROR = 0,
        RZ_FS_COPY_EXISTS_SKIP,
        RZ_FS_COPY_EXISTS_OVERWRITE,
    } exists;

    enum : rz_u8
    {
        RZ_FS_COPY_SYMLINK_FOLLOW = 0,
        RZ_FS_COPY_SYMLINK_SKIP,
        RZ_FS_COPY_SYMLINK_COPY,
    } symlink;

    enum : rz_u8
    {
        RZ_FS_COPY_CONTENT   = 0,
        RZ_FS_COPY_SOFT_LINK = 1,
        RZ_FS_COPY_HARD_LINK = 2,
    } mode;

    // default is include hidden files/directories
    bool exclude_hidden;
    // Recursively copy subdirectories and their content.
    bool recursive;
    // Copy the directory structure, but do not copy any non-directory files.
    bool directories_only;
} RZ_FsCopyOpt;
RZ_DEC bool rz_fs_copy_opt(const RZ_Path src_path, const RZ_Path dst_path, RZ_FsCopyOpt opt);
#    define rz_fs_copy(src_path, dst_path, ...) rz_fs_copy_opt(src_path, dst_path, (RZ_FsCopyOpt){__VA_ARGS__})

typedef struct {
    bool skip_exists;
    // Recursively create a directory and all of its parent components if they are missing.
    bool   all;
    rz_u32 perm;
} RZ_FsCreateDirOpt;
RZ_DEC bool rz_fs_create_dir_opt(const RZ_Path dir_path, RZ_FsCreateDirOpt opt);
#    define rz_fs_create_dir(dir_path, ...) rz_fs_create_dir_opt(dir_path, (RZ_FsCreateDirOpt){__VA_ARGS__});

typedef struct {
    // if path is not exists. dont error (false). and return success (true).
    bool ignore_non_existing;
    bool ignore_access_denied;

    // remove file or directory into the trash (recycle bin).
    // TODO: implement
    bool remove_to_trash;
    // Recursively remove if path is directory and all of its parent components.
    bool all;
} RZ_FsRemoveOpt;
// remove file or empty directory
RZ_DEC bool rz_fs_remove_opt(const RZ_Path path, RZ_FsRemoveOpt opt);
#    define rz_fs_remove(path, ...) rz_fs_remove_opt(path, (RZ_FsRemoveOpt){__VA_ARGS__})

typedef struct {
    bool replace_existing;
    bool ignore_existing;
} RZ_FsRenameOpt;
RZ_DEC bool rz_fs_rename_opt(const RZ_Path old, const RZ_Path new, RZ_FsRenameOpt opt);
#    define rz_fs_rename(old, new, ...) rz_fs_rename_opt(old, new, ((RZ_FsRenameOpt){__VA_ARGS__}))
#    define rz_fs_move_opt              rz_fs_rename_opt
#    define rz_fs_move                  rz_fs_rename

RZ_DEC bool rz_fs_read(const RZ_Path path, RZ_BytesArray *bytes);
RZ_DEC bool rz_fs_read_to_string(const RZ_Path path, RZ_Str *string);

RZ_DEC bool rz_fs_write(const RZ_Path path, const rz_u8 *bytes, rz_usize bytes_len);
#    define rz_fs_write_array(path, array) rz_fs_write(path, (array)->data, (array)->len * sizeof(*(array)->data));

typedef enum : rz_i8
{
    RZ_PATH_MODIF_ERROR = -1,
    RZ_PATH_MODIF_UNCHANGED,
    RZ_PATH_MODIF_CHANGED,
} RZ_PathModifiedStatus;
RZ_DEC RZ_PathModifiedStatus rz_fs_is_modified(const RZ_Path input, const RZ_Path output);

typedef struct {
    // Sets the option for read access.
    bool read;
    // Sets the option for write access.
    bool write;
    // Sets the option for the append mode.
    bool append;
    // Sets the option for truncating a previous file.
    bool truncate;
    // Sets the option to create a new file, or open it if it already exists.
    // if set and .write or .append is not set. rz_fd_open_opt will failed
    bool create;
    // Sets the option to create a new file, failing if it already exists.
    // if set and .write or .append is not set. rz_fd_open_opt will failed
    bool create_new;

    // [for unix only]
    // default to 0644 `RZ_DEFAULT_OPEN_CREATION_PERMISION`
    // [user: read & write] [group: read] [other: read]
    rz_u32 perm;
} RZ_FdOpenOpt;

RZ_DEC RZ_Fd rz_fd_open_opt(const RZ_Path path, RZ_FdOpenOpt opt);

#    define rz_fd_open(path, ...)            rz_fd_open_opt(path, (RZ_FdOpenOpt){.permission = RZ_DEFAULT_OPEN_CREATION_PERMISION __VA_OPT__(, ) __VA_ARGS__})
#    define rz_fd_open_read(path, ...)       rz_fd_open_opt(path, (RZ_FdOpenOpt){.read = true __VA_OPT__(, ) __VA_ARGS__})
#    define rz_fd_open_write(path, ...)      rz_fd_open_opt(path, (RZ_FdOpenOpt){.write = true, .create = true, .truncate = true __VA_OPT__(, ) __VA_ARGS__})

#    define rz_fd_open_cstr(path, ...)       rz_fd_open(rz_path(path), __VA_ARGS__)
#    define rz_fd_open_cstr_read(path, ...)  rz_fd_open_read(rz_path(path), __VA_ARGS__)
#    define rz_fd_open_cstr_write(path, ...) rz_fd_open_write(rz_path(path), __VA_ARGS__)

RZ_DEC bool rz_fd_close(RZ_Fd fd);

/// read buffer from fd. simple wrapper from os API. (`read` on unix, `ReadFile` on windowx)
///
/// @return size of buffer read. if success
/// @return 0 on EOF
/// @return -1 on error. get error message from rz_strerror.
RZ_DEC bool rz_fd_read(RZ_Fd fd, void *dst, rz_usize dstsize, rz_usize *readsize);

/// write buffer into fd. simple wrapper from os API. (`write` on unix, `WriteFile` on windowx)
///
/// @return size of buffer written. if success
/// @return 0 on EOF
/// @return -1 on error. get error message from rz_strerror.
RZ_DEC bool rz_fd_write(RZ_Fd fd, const void *src, rz_usize srclen, rz_usize *writesize);

RZ_DEC bool rz_fd_read_all(RZ_Fd fd, RZ_BytesArray *bytes);
RZ_DEC bool rz_fd_write_all(RZ_Fd fd, const rz_u8 *src, rz_usize srclen);

RZ_DEC RZ_Fd rz_fd_stdin(void);
RZ_DEC RZ_Fd rz_fd_stdout(void);
RZ_DEC RZ_Fd rz_fd_stderr(void);

typedef struct {
    RZ_Fd read;
    RZ_Fd write;
} RZ_FdPipe;
RZ_DEC bool rz_fd_pipe(RZ_FdPipe *pp);

RZ_DEC bool rz_getenv(const char *name, RZ_Str *result_value);

typedef struct RZ_FsDir RZ_FsDir;
typedef struct {
    // if failed or error. this path is empty. check it with rz_path_is_empty
    RZ_Path         path;
    RZ_PathFileType type;
} RZ_FsDirEntry;

RZ_DEC RZ_FsDir *rz_dir_open(const RZ_Path path, RZ_Allocator a);
RZ_DEC void      rz_dir_close(RZ_FsDir *dir);

typedef enum : rz_i8
{
    // encounter erorr while try to get directory entry
    RZ_DIR_ENTRY_ERROR = -1,
    // status stop / finish
    RZ_DIR_ENTRY_STOP = 0,
    // status ok
    RZ_DIR_ENTRY_OK = 1,
} RZ_FsDirEntryStatus;
RZ_DEC RZ_FsDirEntryStatus rz_dir_next(RZ_FsDir *dir, RZ_FsDirEntry *entry);

typedef struct RZ_Walkdir RZ_Walkdir;

typedef struct {
    RZ_Allocator allocator;

    // if depth not provided. it will recursive if recursive true. or just one level if recursive false
    rz_usize depth;
    // if true. it will recursively traverse all sub directories.
    bool recursive;
    // if true. entry hidden file. default is hidden file not included
    bool hidden;
    // if true. entry only file. skip entry a directories.
    bool file_only;
    // if true. always entry directory content first before entry the directory it self.
    // by default. its show directory first, and then the content.
    bool entry_file_first;
    // if set to true. if poll next directory entry encounter error like permission, etc.
    // it will be ignored
    bool ignore_error;
} RZ_WalkdirOpenOpt;
RZ_DEC RZ_Walkdir *rz_walkdir_open_opt(const RZ_Path path, RZ_WalkdirOpenOpt opt);
#    define rz_walkdir_open(path, ...) rz_walkdir_open_opt(path, (RZ_WalkdirOpenOpt){__VA_ARGS__})
RZ_DEC void                rz_walkdir_close(RZ_Walkdir *wd);
RZ_DEC RZ_FsDirEntryStatus rz_walkdir_next(RZ_Walkdir *wd, RZ_FsDirEntry *entry);

#    define rz_walkdir_foreach(wd, entry, status) for (auto status = rz_walkdir_next(wd, entry); status != RZ_DIR_ENTRY_STOP; status = rz_walkdir_next(wd, entry))

#endif /* end of include guard: __RZ_FS_H */
