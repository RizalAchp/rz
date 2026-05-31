#pragma once

#ifndef __RZ_FS_H
#    define __RZ_FS_H
#    include "rz_common.h"
#    include "rz_strings.h"
#    include "rz_time.h"

#    if defined(RZ_OS_WINDOWS)
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

#    define rz_pathbuf_view(path)                 ((const RZ_Path *)path)
#    define rz_pathbuf_ref                        rz_pathbuf_view

RZ_DEC RZ_NODISCARD const char *rz_pathbuf_cstr(RZ_PathBuf *path);

RZ_DEC bool rz_pathbuf_set_filename(RZ_PathBuf *path, RZ_Path filename);
RZ_DEC bool rz_pathbuf_set_filestem(RZ_PathBuf *path, RZ_Path filestem);
RZ_DEC bool rz_pathbuf_set_extension(RZ_PathBuf *path, RZ_Path extension);
RZ_DEC bool rz_pathbuf_add_extension(RZ_PathBuf *path, RZ_Path extension);

RZ_DEC RZ_NODISCARD RZ_PathBuf rz_pathbuf_join(const RZ_PathBuf *path, RZ_Path component);
RZ_DEC void                    rz_pathbuf_push(RZ_PathBuf *path, RZ_Path component);
RZ_DEC bool                    rz_pathbuf_canonicalize(RZ_PathBuf *path);
RZ_DEC bool                    rz_pathbuf_absolute(RZ_PathBuf *path);

RZ_DEC bool rz_path_current_dir(RZ_PathBuf *path);
RZ_DEC bool rz_path_current_exe(RZ_PathBuf *path);
RZ_DEC bool rz_path_home_dir(RZ_PathBuf *path);
RZ_DEC bool rz_path_temp_dir(RZ_PathBuf *path);
RZ_DEC bool rz_path_set_current_dir(const RZ_Path path);

#    define rz_path              rz_sv
#    define rz_path_sized        rz_sv_sized
#    define rz_path_from_pathbuf rz_sv_from_str
#    define rz_path_empty        rz_sv_empty
#    define rz_path_static       rz_sv_static

RZ_DEC RZ_NODISCARD const char *rz_path_cstr(const RZ_Path path, RZ_Allocator allocator);
#    define rz_path_cstr_temp(path)        rz_path_cstr(path, rz_temp_allocator())

#    define rz_pathbuf_clone(pathbuf)      rz_arr_clone(pathbuf)
#    define rz_pathbuf_clone_temp(pathbuf) rz_arr_clone_with_allocator(pathbuf, rz_temp_allocator())

RZ_DEC bool rz_path_filename(const RZ_Path path, RZ_Path *filename);
RZ_DEC bool rz_path_filestem(const RZ_Path path, RZ_Path *filestem);
RZ_DEC bool rz_path_extension(const RZ_Path path, RZ_Path *extension);
RZ_DEC bool rz_path_has_extension(const RZ_Path path, RZ_Path ext);

RZ_DEC bool rz_path_is_absolute(const RZ_Path path);
RZ_DEC bool rz_path_is_relative(const RZ_Path path);

typedef enum : rz_u8
{
    RZ_PATH_FILE_TYPE_UNKNOWN = 0,
    RZ_PATH_FILE_TYPE_FILE,
    RZ_PATH_FILE_TYPE_DIR,
    RZ_PATH_FILE_TYPE_SYMLINK,
} RZ_PathFileType;

#    define rz_path_filetype_is_file(filetype)    ((filetype) == RZ_PATH_FILE_TYPE_FILE)
#    define rz_path_filetype_is_dir(filetype)     ((filetype) == RZ_PATH_FILE_TYPE_DIR)
#    define rz_path_filetype_is_symlink(filetype) ((filetype) == RZ_PATH_FILE_TYPE_SYMLINK)

typedef struct {
    RZ_PathFileType type;
    rz_u64          size;

    RZ_SystemTime modified;
    RZ_SystemTime accessed;
    RZ_SystemTime created;
} RZ_PathMetadata;

RZ_DEC bool rz_path_metadata(const RZ_Path path, RZ_PathMetadata *metadata);
RZ_DEC bool rz_path_symlink_metadata(const RZ_Path path, RZ_PathMetadata *metadata);

RZ_DEC RZ_PathFileType rz_path_filetype(const RZ_Path path);
#    define rz_path_is_file(path)    rz_path_filetype_is_file(rz_path_filetype(path))
#    define rz_path_is_dir(path)     rz_path_filetype_is_dir(rz_path_filetype(path))
#    define rz_path_is_symlink(path) rz_path_filetype_is_symlink(rz_path_filetype(path))

RZ_DEC rz_u64        rz_path_file_size(const RZ_Path path);
RZ_DEC RZ_SystemTime rz_path_modified(const RZ_Path path);
RZ_DEC RZ_SystemTime rz_path_accessed(const RZ_Path path);
RZ_DEC RZ_SystemTime rz_path_created(const RZ_Path path);

RZ_DEC bool rz_path_exists(const RZ_Path path);
RZ_DEC bool rz_path_read_symlink(const RZ_Path path, RZ_PathBuf *symlink_result);

#    define rz_path_read_dir(path) rz_dir_open(path)

typedef enum : rz_i8
{
    RZ_PATH_MODIF_ERROR = -1,
    RZ_PATH_MODIF_UNCHANGED,
    RZ_PATH_MODIF_CHANGED,
} RZ_PathModifiedStatus;
RZ_DEC RZ_PathModifiedStatus rz_path_is_modified(const RZ_Path input, const RZ_Path output);

#    ifndef RZ_DEFAULT_OPEN_CREATION_PERMISION
#        define RZ_DEFAULT_OPEN_CREATION_PERMISION 0644
#    endif

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
    bool create;
    // Sets the option to create a new file, failing if it already exists.
    bool create_new;

    // [for unix only]
    // default to 0644 `RZ_DEFAULT_OPEN_CREATION_PERMISION`
    // [user: read & write] [group: read] [other: read]
    rz_int permission;
} RZ_FdOpenOpt;

RZ_DEC RZ_Fd rz_fd_open_opt(const RZ_Path *path, RZ_FdOpenOpt opt);
#    define rz_fd_open(path, ...)       rz_fd_open_opt(path, (RZ_FdOpenOpt){.permission = RZ_DEFAULT_OPEN_CREATION_PERMISION __VA_OPT__(, ) __VA_ARGS__})
#    define rz_fd_open_read(path, ...)  rz_fd_open_opt(path, (RZ_FdOpenOpt){.read = true __VA_OPT__(, ) __VA_ARGS__})
#    define rz_fd_open_write(path, ...) rz_fd_open_opt(path, (RZ_FdOpenOpt){.write = true, .create = true, .truncate = true __VA_OPT__(, ) __VA_ARGS__})

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

RZ_DEC bool rz_fd_read_all(RZ_Fd fd, RZ_StrBuilder *sb);
RZ_DEC bool rz_fd_write_all(RZ_Fd fd, const char *src, rz_usize srclen);

RZ_DEC RZ_Fd rz_fd_stdin(void);
RZ_DEC RZ_Fd rz_fd_stdout(void);
RZ_DEC RZ_Fd rz_fd_stderr(void);

typedef struct {
    RZ_Fd read;
    RZ_Fd write;
} RZ_FdPipe;
RZ_DEC bool rz_fd_pipe(RZ_FdPipe *pp);

RZ_DEC bool rz_getenv(const char *name, RZ_Str *result_value);

typedef struct RZ_PathDir RZ_PathDir;
typedef struct {
    // if failed or error. this is NULL
    const char     *filename;
    RZ_PathFileType type;

} RZ_PathDirEntry;

RZ_DEC RZ_PathDir *rz_dir_open(const RZ_Path path, RZ_Allocator a);
RZ_DEC bool        rz_dir_next(RZ_PathDir *dir, RZ_PathDirEntry *entry);
RZ_DEC void        rz_dir_close(RZ_PathDir *dir, RZ_Allocator a);

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
    bool directory_file_first;
} RZ_WalkdirOpenOpt;
RZ_DEC RZ_Walkdir *rz_walkdir_open_opt(const RZ_Path path, RZ_WalkdirOpenOpt opt);
#    define rz_walkdir_open(path, ...) rz_walkdir_open_opt(path, (RZ_WalkdirOpenOpt){__VA_ARGS__})
RZ_DEC void rz_walkdir_close(RZ_Walkdir *wd);

typedef struct {
    // if null. error
    const RZ_Path   *filename;
    RZ_PathMetadata *metadata;
} RZ_WalkDirEntry;
RZ_DEC bool rz_walkdir_next(RZ_Walkdir *wd, RZ_WalkDirEntry *entry);

#endif /* end of include guard: __RZ_FS_H */
