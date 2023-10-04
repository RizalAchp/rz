#pragma once

#ifndef CB_H_
#    define CB_H_

#    ifdef CB_TESTS
#        define CB_IMPLEMENTATION
#    endif

#    define CB_ASSERT            assert
#    define CB_REALLOC           realloc
#    define CB_FREE              free
#    define CB_ASSERT_ALLOC(PTR) CB_ASSERT((PTR) && "Buy more RAM lol")

#    include <assert.h>
#    include <ctype.h>
#    include <errno.h>
#    include <stdarg.h>
#    include <stdbool.h>
#    include <stdint.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>

#    if defined(__APPLE__) || defined(__MACH__)
#        define CB_MACOS
#        define CB_DEFAULT_PLATFORM PLATFORM_MACOS
#        define CB_PATH_SEPARATOR   ":"
#        define CB_DIR_SEPARATOR    '/'
#        define CB_LINE_END         "\n"
#    elif defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__)
#        define CB_WINDOWS
#        define CB_DEFAULT_PLATFORM PLATFORM_WINDOWS
#        define WIN32_LEAN_AND_MEAN
#        include <direct.h>
#        include <shellapi.h>
#        include <windows.h>
#        define getcwd            _getcwd
#        define access            _access
#        define F_OK              0
#        define CB_PATH_SEPARATOR ";"
#        define CB_DIR_SEPARATOR  '\\'
#        define CB_LINE_END       "\r\n"
#    elif defined(__linux__) && defined(__unix__)
#        define CB_UNIX
#        define CB_DEFAULT_PLATFORM PLATFORM_UNIX
#        include <dirent.h>
#        include <fcntl.h>
#        include <limits.h>
#        include <sys/stat.h>
#        include <sys/types.h>
#        include <sys/wait.h>
#        include <unistd.h>
#        define MAX_PATH          PATH_MAX
#        define CB_PATH_SEPARATOR ":"
#        define CB_DIR_SEPARATOR  '/'
#        define CB_LINE_END       "\n"
#    else
#        error "Platform: Unknown platform, not supported platform"
#    endif

#    ifndef CB_FNDEF
#        define CB_FNDEF
#    endif

#    define CB_ARR_LEN(array)        (sizeof(array) / sizeof(array[0]))
#    define CB_ARR_GET(array, index) (CB_ASSERT(index >= 0), CB_ASSERT(index < CB_ARR_LEN(array)), array[index])

#    define CB_ERROR(...)            cb_log(CB_LOG_ERROR, __VA_ARGS__)
#    define CB_INFO(...)             cb_log(CB_LOG_INFO, __VA_ARGS__)
#    define CB_WARNING(...)          cb_log(CB_LOG_WARNING, __VA_ARGS__)

#    define CB_IS_SPACE(x)           ((x) == ' ' || (x) == '\t' || (x) == '\n' || (x) == '\r')

// Initial capacity of a dynamic array
#    define CB_DA_INIT_CAP 256
// Append an item to a dynamic array
#    define cb_da_first(da) (CB_ASSERT((da)->count), &(da)->items[0])
#    define cb_da_last(da)  (CB_ASSERT((da)->count), &(da)->items[(da)->count - 1])
#    define cb_da_append(da, item)                                                               \
        do {                                                                                     \
            if ((da)->count >= (da)->capacity) {                                                 \
                (da)->capacity = (da)->capacity == 0 ? CB_DA_INIT_CAP : (da)->capacity * 2;      \
                (da)->items    = CB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); \
                CB_ASSERT_ALLOC((da)->items);                                                    \
            }                                                                                    \
            (da)->items[(da)->count++] = (item);                                                 \
        } while (0)

#    define cb_da_init(da, cap)                                             \
        do {                                                                \
            (da)->count    = 0;                                             \
            (da)->capacity = (cap < CB_DA_INIT_CAP) ? CB_DA_INIT_CAP : cap; \
            size_t sz      = (da)->capacity * sizeof((da)->items[0]);       \
            (da)->items    = CB_REALLOC(NULL, sz);                          \
            CB_ASSERT_ALLOC((da)->items);                                   \
        } while (0)
#    define cb_da_free(da)        \
        do {                      \
            CB_FREE((da).items);  \
            (da).items    = NULL; \
            (da).capacity = 0;    \
            (da).count    = 0;    \
        } while (0)

// Append several items to a dynamic array
#    define cb_da_append_many(da, new_items, new_items_count)                                     \
        do {                                                                                      \
            if ((da)->count + new_items_count > (da)->capacity) {                                 \
                if ((da)->capacity == 0) (da)->capacity = CB_DA_INIT_CAP;                         \
                while ((da)->count + new_items_count > (da)->capacity) (da)->capacity *= 2;       \
                (da)->items = CB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));     \
                CB_ASSERT_ALLOC((da)->items);                                                     \
            }                                                                                     \
            memcpy((da)->items + (da)->count, new_items, new_items_count * sizeof(*(da)->items)); \
            (da)->count += new_items_count;                                                       \
        } while (0)

#    define cb_return_defer(value) \
        do {                       \
            result = (value);      \
            goto defer;            \
        } while (0)

// TODO: add MinGW support
#    ifndef CB_REBUILD_ARGS
#        if CB_WINDOWS
#            if defined(__GNUC__)
#                define CB_REBUILD_ARGS(binary_path, source_path) "gcc", "-o", binary_path, source_path
#            elif defined(__clang__)
#                define CB_REBUILD_ARGS(binary_path, source_path) "clang", "-o", binary_path, source_path
#            elif defined(_MSC_VER)
#                define CB_REBUILD_ARGS(binary_path, source_path) "cl.exe", source_path
#            endif
#        else
#            define CB_REBUILD_ARGS(binary_path, source_path) "cc", "-o", binary_path, source_path
#        endif
#    endif
#    define CB_REBUILD_SELF(argc, argv)                                                     \
        do {                                                                                \
            const char *source_path = __FILE__;                                             \
            assert(argc >= 1);                                                              \
            const char *binary_path       = argv[0];                                        \
                                                                                            \
            int         rebuild_is_needed = cb_needs_rebuild(binary_path, &source_path, 1); \
            if (rebuild_is_needed < 0) exit(1);                                             \
            if (rebuild_is_needed) {                                                        \
                cb_str_builder_t sb = {0};                                                  \
                cb_sb_append_cstr(&sb, binary_path);                                        \
                cb_sb_append_cstr(&sb, ".old");                                             \
                cb_sb_append_null(&sb);                                                     \
                                                                                            \
                if (!cb_rename(binary_path, sb.items)) exit(1);                             \
                cb_cmd_t rebuild = {0};                                                     \
                cb_cmd_append(&rebuild, CB_REBUILD_ARGS(binary_path, source_path));         \
                bool rebuild_succeeded = cb_cmd_run_sync(rebuild);                          \
                cb_cmd_free(rebuild);                                                       \
                if (!rebuild_succeeded) {                                                   \
                    cb_rename(sb.items, binary_path);                                       \
                    exit(1);                                                                \
                }                                                                           \
                                                                                            \
                cb_cmd_t cmd = {0};                                                         \
                cb_da_append_many(&cmd, argv, argc);                                        \
                if (!cb_cmd_run_sync(cmd)) exit(1);                                         \
                exit(0);                                                                    \
            }                                                                               \
        } while (0)

typedef enum {
    CB_ERR,
    CB_OK,
    CB_FAIL,
} cb_status_t;

typedef enum {
    CB_LOG_INFO,
    CB_LOG_WARNING,
    CB_LOG_ERROR,
    CB_LOG_LEVEL_MAX,
} cb_log_level_t;

typedef enum {
    CB_FILE_REGULAR = 0,
    CB_FILE_DIRECTORY,
    CB_FILE_SYMLINK,
    CB_FILE_OTHER,
} cb_file_type_t;

// clang-format off
typedef enum { CB_SUBCMD_NOOP,      CB_SUBCMD_BUILD,            CB_SUBCMD_CONFIG,           CB_SUBCMD_TESTS,    CB_SUBCMD_MAX   } cb_subcmd_t;
typedef enum { CB_BUILD_TYPE_DEBUG, CB_BUILD_TYPE_RELEASE,      CB_BUILD_TYPE_RELEASEDEBUG, CB_BUILD_TYPE_MAX                   } cb_build_t;
typedef enum { CB_PLATFORM_UNKNOWN, CB_PLATFORM_WINDOWS,        CB_PLATFORM_MACOS,          CB_PLATFORM_UNIX,   CB_PLATFORM_MAX } cb_platform_t;
typedef enum { CB_ARCH_UNKNOWN,     CB_ARCH_X64,                CB_ARCH_X86, CB_ARCH_ARM64, CB_ARCH_ARM32,      CB_ARCH_MAX     } cb_arch_t;
typedef enum { CB_COMPILER_UNKNOWN, CB_COMPILER_CLANG,          CB_COMPILER_GNU,            CB_COMPILER_MSVC,   CB_COMPILER_MAX } cb_compiler_t;
typedef enum { CB_PROGRAM_UNKNOWN,  CB_PROGRAM_C,               CB_PROGRAM_CPP,             CB_PROGRAM_MAX                      } cb_program_t;
typedef enum { CB_TARGET_TYPE_EXEC, CB_TARGET_TYPE_STATIC_LIB,  CB_TARGET_TYPE_DYNAMIC_LIB, CB_TARGET_TYPE_SYSTEM_LIB } cb_target_type_t;
// clang-format on

CB_FNDEF void cb_log(cb_log_level_t level, const char *fmt, ...);
// It is an equivalent of shift command from bash. It basically pops a command line
// argument from the beginning.
CB_FNDEF char                  *cb_shift_args(int *argc, char ***argv);

typedef struct cb_path_t        cb_path_t;
typedef struct cb_file_paths_t  cb_file_paths_t;
typedef struct cb_str_builder_t cb_str_builder_t;
typedef struct cb_procs_t       cb_procs_t;
// A command - the main workhorse of Cb. Cb is all about building commands an running them
typedef struct cb_cmd_t     cb_cmd_t;
typedef struct cb_strview_t cb_strview_t;
typedef struct cb_set_t     cb_set_t;

typedef struct cb_config_t  cb_config_t;
typedef struct cb_target_t  cb_target_t;
typedef struct cb_t         cb_t;

/// cb_path_t /////////////////////////////////////////////
///
CB_FNDEF cb_path_t    cb_path(const char *str);
CB_FNDEF cb_path_t    cb_path_parts(const char *str, size_t len);
CB_FNDEF cb_strview_t cb_path_extension(cb_path_t *path);
CB_FNDEF cb_strview_t cb_path_filename(cb_path_t *path);
CB_FNDEF cb_status_t  cb_path_append(cb_path_t *path, cb_strview_t other);
CB_FNDEF cb_status_t  cb_path_append_cstr(cb_path_t *path, const char *other);
CB_FNDEF bool         cb_path_with_extension(cb_path_t *path, const char *ext);
CB_FNDEF bool         cb_path_has_extension(cb_path_t *path);
#    define cb_path_is_regular_file(p) (cb_get_file_type((p)->data) == CB_FILE_REGULAR)
#    define cb_path_is_directory(p)    (cb_get_file_type((p)->data) == CB_FILE_DIRECTORY)
#    define cb_path_is_symlink(p)      (cb_get_file_type((p)->data) == CB_FILE_SYMLINK)
#    define cb_path_exists(p)          cb_file_exists((p)->data)
#    define cb_path_to_cstr(p)         ((p)->data[(p)->count] = 0, (p)->data)
#    define cb_path_to_strview(p)      cb_sv_from_parts((p)->data, (p)->count)

/// os operation /////////////////////////////////////////////
///
CB_FNDEF bool           cb_mkdir_if_not_exists(const char *path);
CB_FNDEF bool           cb_copy_file(const char *src_path, const char *dst_path);
CB_FNDEF bool           cb_copy_directory_recursively(const char *src_path, const char *dst_path);
CB_FNDEF bool           cb_read_entire_dir(const char *parent, cb_file_paths_t *children);
CB_FNDEF bool           cb_write_entire_file(const char *path, void *data, size_t size);
CB_FNDEF bool           cb_rename(const char *old_path, const char *new_path);
CB_FNDEF int            cb_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count);
CB_FNDEF int            cb_file_exists(const char *file_path);
CB_FNDEF cb_file_type_t cb_get_file_type(const char *path);
CB_FNDEF bool           cb_read_entire_file(const char *path, cb_str_builder_t *sb);

/// cb_str_builder_t /////////////////////////////////////////////
///
#    define cb_sb_append_buf(sb, buf, size) /* Append a sized buffer to a string builder */ cb_da_append_many(sb, buf, size)
#    define cb_sb_append_cstr(sb, cstr)     /* Append a NULL-terminated string to a string builder */ \
        do {                                                                                          \
            const char *s = (cstr);                                                                   \
            size_t      n = strlen(s);                                                                \
            cb_da_append_many(sb, s, n);                                                              \
        } while (0)
// Append a single NULL character at the end of a string builder. So then you can use it a NULL-terminated C string
#    define cb_sb_append_null(sb) cb_da_append_many(sb, "", 1)
#    define cb_sb_free(sb)        /* Free the memory allocated by a string builder */ CB_FREE((sb).items)

// Process handle
#    ifdef CB_WINDOWS
typedef HANDLE cb_proc_t;
#        define CB_INVALID_PROC INVALID_HANDLE_VALUE
#    else
typedef int cb_proc_t;
#        define CB_INVALID_PROC (-1)
#    endif  // CB_WINDOWS

/// cb_proc_t | cb_procs_t /////////////////////////////////////////////
///
// Wait until the process has finished
CB_FNDEF bool cb_procs_wait(cb_procs_t procs);
CB_FNDEF bool cb_proc_wait(cb_proc_t proc);

/// cb_cmd_t /////////////////////////////////////////////
///
// Render a string representation of a command into a string builder. Keep in mind the the
// string builder is not NULL-terminated by default. Use cb_sb_append_null if you plan to
// use it as a C string.
void cb_cmd_render(cb_cmd_t cmd, cb_str_builder_t *render);
#    define cb_cmd_append(cmd, ...) cb_da_append_many(cmd, ((const char *[]){__VA_ARGS__}), (sizeof((const char *[]){__VA_ARGS__}) / sizeof(const char *)))
#    define cb_cmd_free(cmd)        CB_FREE(cmd.items)
CB_FNDEF cb_proc_t cb_cmd_run_async(cb_cmd_t cmd);
bool               cb_cmd_run_sync(cb_cmd_t cmd);

/// temp allocator /////////////////////////////////////////////
///
#    ifndef CB_TEMP_CAPACITY
#        define CB_TEMP_CAPACITY (8 * 1024 * 1024)
#    endif  // CB_TEMP_CAPACITY
char  *cb_temp_strdup(const char *cstr);
void  *cb_temp_alloc(size_t size);
char  *cb_temp_sprintf(const char *format, ...);
void   cb_temp_reset(void);
size_t cb_temp_save(void);
void   cb_temp_rewind(size_t checkpoint);

#    define cb_temp_to_cstr(s)                                                         \
        do {                                                                           \
            char *result = cb_temp_alloc(sv.count + 1);                                \
            CB_ASSERT(result != NULL && "Extend the size of the temporary allocator"); \
            memcpy(result, sv.data, sv.count);                                         \
            result[sv.count] = '\0';                                                   \
            return result;                                                             \
        } while (0)

/// cb_strview_t /////////////////////////////////////////////
///
#    define cb_sv(cstr)             ((cb_strview_t){.data = (const char *)cstr, .count = strlen(cstr)})
#    define cb_sv_from_parts(D, C)  ((cb_strview_t){.count = C, .data = D})
#    define cb_sv_start_with(sv, c) ((sv)->data[0] == c)
#    define cb_sv_end_with(sv, c)   ((sv)->data[(sv)->count - 1] == c)

cb_strview_t cb_sv_chop_right_by_delim(cb_strview_t *sv, char delim);
cb_strview_t cb_sv_chop_left_by_delim(cb_strview_t *sv, char delim);
cb_strview_t cb_sv_trim(cb_strview_t sv);
bool         cb_sv_eq(cb_strview_t a, cb_strview_t b);
cb_strview_t cb_sv_from_cstr(const char *cstr);

#    define SVFmt     "%.*s"
#    define SVArg(sv) (int)(sv).count, (sv).data

#    ifdef CB_WINDOWS
struct dirent {
    char d_name[MAX_PATH + 1];
};
typedef struct DIR DIR;
DIR               *opendir(const char *dirpath);
struct dirent     *readdir(DIR *dirp);
int                closedir(DIR *dirp);
#    endif  // CB_WINDOWS

/// cb_target_t /////////////////////////////////////////////
///
cb_target_t cb_target_create(cb_strview_t name, cb_target_type_t type);
cb_status_t cb_target_add_sources_with_ext(cb_target_t *tg, const char *dir, const char *ext, bool recursive);
cb_status_t cb_target_add_sources(cb_target_t *tg, ...);
cb_status_t cb_target_add_flags(cb_target_t *tg, ...);
cb_status_t cb_target_add_includes(cb_target_t *tg, ...);
cb_status_t cb_target_add_defines(cb_target_t *tg, ...);
cb_status_t cb_target_add_precompiled_header(cb_target_t *tg, ...);
cb_status_t cb_target_link_library(cb_target_t *tg, ...);
bool        cb_target_need_rebuild(cb_target_t *tg);

/// cb_t /////////////////////////////////////////////
///
// init cb_t returning pointer of the `cb_t`, if error, return `NULL`
// `cb_t` is created in head, it should `cb_deinit` after using it
cb_t       *cb_init(int argc, char **argv);
cb_status_t cb_run(cb_t *cb);
void        cb_deinit(cb_t *cb);

// user self implementation function for setting up builder on `build` subcommand
cb_status_t build(cb_t *cb);
// user self implementation function for setting up tester on `tests` subcommand
cb_status_t tests(cb_t *cb);
#endif  // CB_H_

////////////////////////////////////////////////////////////////////////////////
#ifdef CB_IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

static size_t      cb_temp_size              = 0;
static char        cb_temp[CB_TEMP_CAPACITY] = {0};
static const char *CB_LOG_LEVEL_DISPLAY[]    = {"INFO", "WARN", "ERROR"};

struct cb_path_t {
    size_t count;
    char   data[MAX_PATH];
};

struct cb_file_paths_t {
    size_t       capacity;
    size_t       count;
    const char **items;
};

struct cb_cmd_t {
    size_t       capacity;
    size_t       count;
    const char **items;
};

struct cb_procs_t {
    size_t     capacity;
    size_t     count;
    cb_proc_t *items;
};

struct cb_str_builder_t {
    size_t capacity;
    size_t count;
    char  *items;
};

struct cb_strview_t {
    size_t      count;
    const char *data;
};

typedef struct {
    cb_strview_t item;
    int64_t      hash;
} cb_set_item_t;
struct cb_set_t {
    size_t         capacity;
    size_t         count;
    cb_set_item_t *items;
};

struct cb_config_t {
    cb_build_t     build_type;
    cb_platform_t  platform;
    cb_arch_t      arch;
    cb_compiler_t  compiler_type;
    cb_program_t   program_type;
    cb_log_level_t log_level;

    cb_path_t      project_path;
    cb_path_t      build_path;
    cb_path_t      build_artifact_path;
    cb_path_t      compiler_path;
};

struct cb_target_t {
    cb_target_type_t type;
    cb_strview_t     name;
    cb_path_t        output_dir;

    cb_set_t         flags;
    cb_file_paths_t  sources;
};

struct cb_t {
    bool         has_config;
    cb_subcmd_t  subcmd;

    size_t       capacity;
    size_t       count;
    cb_target_t *items;
};

cb_path_t cb_path(const char *str) { return cb_path_parts(str, strlen(str)); }
cb_path_t cb_path_parts(const char *str, size_t len) {
    CB_ASSERT((len < MAX_PATH) && "len of path is greather than MAX_PATH");
    cb_path_t p = {0};
    memset(p.data, 0, sizeof(p.data));
    p.count = len;
    memcpy(p.data, str, len);
    return p;
}
cb_strview_t cb_path_extension(cb_path_t *path) {
    cb_strview_t p = cb_sv_from_parts(path->data, path->count);
    return cb_sv_chop_right_by_delim(&p, '.');
}

cb_strview_t cb_path_filename(cb_path_t *path) {
    cb_strview_t temp = cb_sv_from_parts(path->data, path->count);
    if (cb_sv_end_with(&temp, CB_DIR_SEPARATOR)) temp.count -= 1;
    return cb_sv_chop_right_by_delim(&temp, CB_DIR_SEPARATOR);
}

cb_status_t cb_path_append(cb_path_t *path, cb_strview_t other) {
    if ((other.count + path->count) >= MAX_PATH) return CB_ERR;
    if (!cb_sv_end_with(path, CB_DIR_SEPARATOR)) {
        path->data[path->count] = '/';
        path->count++;
    }
    memcpy(&path->data[path->count], other.data, other.count);
    path->count             = path->count + other.count;
    path->data[path->count] = 0;
    return CB_OK;
}

cb_status_t cb_path_append_cstr(cb_path_t *path, const char *other) { return cb_path_append(path, cb_sv_from_cstr(other)); }

bool        cb_path_with_extension(cb_path_t *path, const char *ext) {
    size_t       ext_len = strlen(ext);
    cb_strview_t tempp   = cb_sv_from_parts(path->data, path->count);
    cb_strview_t p       = cb_sv_chop_right_by_delim(&tempp, '.');
    if (p.count == 0)
        return false;  // doests has extension
                       //
    path->data[tempp.count] = '.';
    memcpy(path->data + (tempp.count + 1), ext, ext_len);
    path->count             = (tempp.count + 1) + ext_len;
    path->data[path->count] = 0;
    return true;
}
bool cb_path_has_extension(cb_path_t *path) { return cb_path_extension(path).count != 0; }

bool cb_mkdir_if_not_exists(const char *path) {
#    ifdef CB_WINDOWS
    int result = mkdir(path);
#    else
    int    result   = mkdir(path, 0755);
#    endif
    if (result < 0) {
        if (errno == EEXIST) {
            CB_INFO("directory `%s` already exists", path);
            return true;
        }
        CB_ERROR("could not create directory `%s`: %s", path, strerror(errno));
        return false;
    }

    CB_INFO("created directory `%s`", path);
    return true;
}

bool cb_copy_file(const char *src_path, const char *dst_path) {
    CB_INFO("copying %s -> %s", src_path, dst_path);
#    ifdef CB_WINDOWS
    if (!CopyFile(src_path, dst_path, FALSE)) {
        CB_ERROR("Could not copy file: %lu", GetLastError());
        return false;
    }
    return true;
#    else
    int    src_fd   = -1;
    int    dst_fd   = -1;
    size_t buf_size = 32 * 1024;
    char  *buf      = CB_REALLOC(NULL, buf_size);
    CB_ASSERT_ALLOC(buf != NULL);
    bool result = true;

    src_fd      = open(src_path, O_RDONLY);
    if (src_fd < 0) {
        CB_ERROR("Could not open file %s: %s", src_path, strerror(errno));
        cb_return_defer(false);
    }

    struct stat src_stat;
    if (fstat(src_fd, &src_stat) < 0) {
        CB_ERROR("Could not get mode of file %s: %s", src_path, strerror(errno));
        cb_return_defer(false);
    }

    dst_fd = open(dst_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
    if (dst_fd < 0) {
        CB_ERROR("Could not create file %s: %s", dst_path, strerror(errno));
        cb_return_defer(false);
    }

    for (;;) {
        ssize_t n = read(src_fd, buf, buf_size);
        if (n == 0) break;
        if (n < 0) {
            CB_ERROR("Could not read from file %s: %s", src_path, strerror(errno));
            cb_return_defer(false);
        }
        char *buf2 = buf;
        while (n > 0) {
            ssize_t m = write(dst_fd, buf2, n);
            if (m < 0) {
                CB_ERROR("Could not write to file %s: %s", dst_path, strerror(errno));
                cb_return_defer(false);
            }
            n -= m;
            buf2 += m;
        }
    }

defer:
    free(buf);
    close(src_fd);
    close(dst_fd);
    return result;
#    endif
}

void cb_cmd_render(cb_cmd_t cmd, cb_str_builder_t *render) {
    for (size_t i = 0; i < cmd.count; ++i) {
        const char *arg = cmd.items[i];
        if (arg == NULL) break;
        if (i > 0) cb_sb_append_cstr(render, " ");
        if (!strchr(arg, ' ')) {
            cb_sb_append_cstr(render, arg);
        } else {
            cb_da_append(render, '\'');
            cb_sb_append_cstr(render, arg);
            cb_da_append(render, '\'');
        }
    }
}

cb_proc_t cb_cmd_run_async(cb_cmd_t cmd) {
    if (cmd.count < 1) {
        CB_ERROR("Could not run empty command");
        return CB_INVALID_PROC;
    }

    cb_str_builder_t sb = {0};
    cb_cmd_render(cmd, &sb);
    cb_sb_append_null(&sb);
    CB_INFO("CMD: %s", sb.items);
    cb_sb_free(sb);
    memset(&sb, 0, sizeof(sb));

#    ifdef CB_WINDOWS
    // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
    // TODO: check for errors in GetStdHandle
    siStartInfo.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // TODO: use a more reliable rendering of the command instead of cmd_render
    // cmd_render is for logging primarily
    cb_cmd_render(cmd, &sb);
    cb_sb_append_null(&sb);
    BOOL bSuccess = CreateProcessA(NULL, sb.items, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    cb_sb_free(sb);

    if (!bSuccess) {
        CB_ERROR("Could not create child process: %lu", GetLastError());
        return CB_INVALID_PROC;
    }

    CloseHandle(piProcInfo.hThread);

    return piProcInfo.hProcess;
#    else
    pid_t cpid = fork();
    if (cpid < 0) {
        CB_ERROR("Could not fork child process: %s", strerror(errno));
        return CB_INVALID_PROC;
    }

    if (cpid == 0) {
        // NOTE: This leaks a bit of memory in the child process.
        // But do we actually care? It's a one off leak anyway...
        cb_cmd_t cmd_null = {0};
        cb_da_append_many(&cmd_null, cmd.items, cmd.count);
        cb_cmd_append(&cmd_null, NULL);

        if (execvp(cmd.items[0], (char *const *)cmd_null.items) < 0) {
            CB_ERROR("Could not exec child process: %s", strerror(errno));
            exit(1);
        }
        CB_ASSERT(0 && "unreachable");
    }

    return cpid;
#    endif
}

bool cb_procs_wait(cb_procs_t procs) {
    bool success = true;
    for (size_t i = 0; i < procs.count; ++i) {
        success = cb_proc_wait(procs.items[i]) && success;
    }
    return success;
}

bool cb_proc_wait(cb_proc_t proc) {
    if (proc == CB_INVALID_PROC) return false;

#    ifdef CB_WINDOWS
    DWORD result = WaitForSingleObject(proc,     // HANDLE hHandle,
                                       INFINITE  // DWORD  dwMilliseconds
    );

    if (result == WAIT_FAILED) {
        CB_ERROR("could not wait on child process: %lu", GetLastError());
        return false;
    }

    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) {
        CB_ERROR("could not get process exit code: %lu", GetLastError());
        return false;
    }

    if (exit_status != 0) {
        CB_ERROR("command exited with exit code %lu", exit_status);
        return false;
    }

    CloseHandle(proc);

    return true;
#    else
    for (;;) {
        int wstatus = 0;
        if (waitpid(proc, &wstatus, 0) < 0) {
            CB_ERROR("could not wait on command (pid %d): %s", proc, strerror(errno));
            return false;
        }

        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                CB_ERROR("command exited with exit code %d", exit_status);
                return false;
            }

            break;
        }

        if (WIFSIGNALED(wstatus)) {
            CB_ERROR("command process was terminated by %s", strsignal(WTERMSIG(wstatus)));
            return false;
        }
    }

    return true;
#    endif
}

bool cb_cmd_run_sync(cb_cmd_t cmd) {
    cb_proc_t p = cb_cmd_run_async(cmd);
    if (p == CB_INVALID_PROC) return false;
    return cb_proc_wait(p);
}

char *cb_shift_args(int *argc, char ***argv) {
    CB_ASSERT(*argc > 0);
    char *result = **argv;
    (*argv) += 1;
    (*argc) -= 1;
    return result;
}

void cb_log(cb_log_level_t level, const char *fmt, ...) {
    fprintf(stderr, "[%s]: ", CB_LOG_LEVEL_DISPLAY[level % CB_LOG_LEVEL_MAX]);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

bool cb_read_entire_dir(const char *parent, cb_file_paths_t *children) {
    bool result = true;
    DIR *dir    = NULL;

    dir         = opendir(parent);
    if (dir == NULL) {
        CB_ERROR("Could not open directory %s: %s", parent, strerror(errno));
        cb_return_defer(false);
    }

    errno              = 0;
    struct dirent *ent = readdir(dir);
    while (ent != NULL) {
        cb_da_append(children, cb_temp_strdup(ent->d_name));
        ent = readdir(dir);
    }

    if (errno != 0) {
        CB_ERROR("Could not read directory %s: %s", parent, strerror(errno));
        cb_return_defer(false);
    }

defer:
    if (dir) closedir(dir);
    return result;
}

bool cb_write_entire_file(const char *path, void *data, size_t size) {
    bool  result = true;

    FILE *f      = fopen(path, "wb");
    if (f == NULL) {
        CB_ERROR("Could not open file %s for writing: %s\n", path, strerror(errno));
        cb_return_defer(false);
    }

    //           len
    //           v
    // aaaaaaaaaa
    //     ^
    //     data

    char *buf = data;
    while (size > 0) {
        size_t n = fwrite(buf, 1, size, f);
        if (ferror(f)) {
            CB_ERROR("Could not write into file %s: %s\n", path, strerror(errno));
            cb_return_defer(false);
        }
        size -= n;
        buf += n;
    }

defer:
    if (f) fclose(f);
    return result;
}

cb_file_type_t cb_get_file_type(const char *path) {
#    ifdef CB_WINDOWS
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        CB_ERROR("Could not get file attributes of %s: %lu", path, GetLastError());
        return -1;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY) return CB_FILE_DIRECTORY;
    // TODO: detect symlinks on Windows (whatever that means on Windows anyway)
    return CB_FILE_REGULAR;
#    else   // CB_WINDOWS
    struct stat statbuf;
    if (stat(path, &statbuf) < 0) {
        CB_ERROR("Could not get stat of %s: %s", path, strerror(errno));
        return -1;
    }

    switch (statbuf.st_mode & S_IFMT) {
        case S_IFDIR: return CB_FILE_DIRECTORY;
        case S_IFREG: return CB_FILE_REGULAR;
        case S_IFLNK: return CB_FILE_SYMLINK;
        default: return CB_FILE_OTHER;
    }
#    endif  // CB_WINDOWS
}

bool cb_copy_directory_recursively(const char *src_path, const char *dst_path) {
    bool             result          = true;
    cb_file_paths_t  children        = {0};
    cb_str_builder_t src_sb          = {0};
    cb_str_builder_t dst_sb          = {0};
    size_t           temp_checkpoint = cb_temp_save();

    cb_file_type_t   type            = cb_get_file_type(src_path);
    if (type < 0) return false;

    switch (type) {
        case CB_FILE_DIRECTORY: {
            if (!cb_mkdir_if_not_exists(dst_path)) cb_return_defer(false);
            if (!cb_read_entire_dir(src_path, &children)) cb_return_defer(false);

            for (size_t i = 0; i < children.count; ++i) {
                if (strcmp(children.items[i], ".") == 0) continue;
                if (strcmp(children.items[i], "..") == 0) continue;

                src_sb.count = 0;
                cb_sb_append_cstr(&src_sb, src_path);
                cb_sb_append_cstr(&src_sb, "/");
                cb_sb_append_cstr(&src_sb, children.items[i]);
                cb_sb_append_null(&src_sb);

                dst_sb.count = 0;
                cb_sb_append_cstr(&dst_sb, dst_path);
                cb_sb_append_cstr(&dst_sb, "/");
                cb_sb_append_cstr(&dst_sb, children.items[i]);
                cb_sb_append_null(&dst_sb);

                if (!cb_copy_directory_recursively(src_sb.items, dst_sb.items)) {
                    cb_return_defer(false);
                }
            }
        } break;

        case CB_FILE_REGULAR: {
            if (!cb_copy_file(src_path, dst_path)) {
                cb_return_defer(false);
            }
        } break;

        case CB_FILE_SYMLINK: {
            CB_WARNING("TODO: Copying symlinks is not supported yet");
        } break;

        case CB_FILE_OTHER: {
            CB_ERROR("Unsupported type of file %s", src_path);
            cb_return_defer(false);
        } break;

        default: CB_ASSERT(0 && "unreachable");
    }

defer:
    cb_temp_rewind(temp_checkpoint);
    cb_da_free(src_sb);
    cb_da_free(dst_sb);
    cb_da_free(children);
    return result;
}

char *cb_temp_strdup(const char *cstr) {
    size_t n      = strlen(cstr);
    char  *result = cb_temp_alloc(n + 1);
    CB_ASSERT(result != NULL && "Increase CB_TEMP_CAPACITY");
    memcpy(result, cstr, n);
    result[n] = '\0';
    return result;
}

void *cb_temp_alloc(size_t size) {
    if (cb_temp_size + size > CB_TEMP_CAPACITY) return NULL;
    void *result = &cb_temp[cb_temp_size];
    cb_temp_size += size;
    return result;
}

char *cb_temp_sprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsnprintf(NULL, 0, format, args);
    va_end(args);

    CB_ASSERT(n >= 0);
    char *result = cb_temp_alloc(n + 1);
    CB_ASSERT(result != NULL && "Extend the size of the temporary allocator");
    // TODO: use proper arenas for the temporary allocator;
    va_start(args, format);
    vsnprintf(result, n + 1, format, args);
    va_end(args);

    return result;
}

void   cb_temp_reset(void) { cb_temp_size = 0; }
size_t cb_temp_save(void) { return cb_temp_size; }
void   cb_temp_rewind(size_t checkpoint) { cb_temp_size = checkpoint; }

int    cb_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count) {
#    ifdef CB_WINDOWS
    BOOL   bSuccess;

    HANDLE output_path_fd = CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (output_path_fd == INVALID_HANDLE_VALUE) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (GetLastError() == ERROR_FILE_NOT_FOUND) return 1;
        CB_ERROR("Could not open file %s: %lu", output_path, GetLastError());
        return -1;
    }
    FILETIME output_path_time;
    bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
    CloseHandle(output_path_fd);
    if (!bSuccess) {
        CB_ERROR("Could not get time of %s: %lu", output_path, GetLastError());
        return -1;
    }

    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path    = input_paths[i];
        HANDLE      input_path_fd = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if (input_path_fd == INVALID_HANDLE_VALUE) {
            // NOTE: non-existing input is an error cause it is needed for building in the first place
            CB_ERROR("Could not open file %s: %lu", input_path, GetLastError());
            return -1;
        }
        FILETIME input_path_time;
        bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
        CloseHandle(input_path_fd);
        if (!bSuccess) {
            CB_ERROR("Could not get time of %s: %lu", input_path, GetLastError());
            return -1;
        }

        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (CompareFileTime(&input_path_time, &output_path_time) == 1) return 1;
    }

    return 0;
#    else
    struct stat statbuf = {0};

    if (stat(output_path, &statbuf) < 0) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (errno == ENOENT) return 1;
        CB_ERROR("could not stat %s: %s", output_path, strerror(errno));
        return -1;
    }
    int output_path_time = statbuf.st_mtime;

    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path = input_paths[i];
        if (stat(input_path, &statbuf) < 0) {
            // NOTE: non-existing input is an error cause it is needed for building in the first place
            CB_ERROR("could not stat %s: %s", input_path, strerror(errno));
            return -1;
        }
        int input_path_time = statbuf.st_mtime;
        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (input_path_time > output_path_time) return 1;
    }

    return 0;
#    endif
}

bool cb_rename(const char *old_path, const char *new_path) {
    CB_INFO("renaming %s -> %s", old_path, new_path);
#    ifdef CB_WINDOWS
    if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) {
        CB_ERROR("could not rename %s to %s: %lu", old_path, new_path, GetLastError());
        return false;
    }
#    else
    if (rename(old_path, new_path) < 0) {
        CB_ERROR("could not rename %s to %s: %s", old_path, new_path, strerror(errno));
        return false;
    }
#    endif  // CB_WINDOWS
    return true;
}

bool cb_read_entire_file(const char *path, cb_str_builder_t *sb) {
    bool   result   = true;
    size_t buf_size = 32 * 1024;
    char  *buf      = CB_REALLOC(NULL, buf_size);
    CB_ASSERT_ALLOC(buf);

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        CB_ERROR("Could not open %s for reading: %s", path, strerror(errno));
        cb_return_defer(false);
    }

    size_t n = fread(buf, 1, buf_size, f);
    while (n > 0) {
        cb_sb_append_buf(sb, buf, n);
        n = fread(buf, 1, buf_size, f);
    }
    if (ferror(f)) {
        CB_ERROR("Could not read %s: %s\n", path, strerror(errno));
        cb_return_defer(false);
    }

defer:
    CB_FREE(buf);
    if (f) fclose(f);
    return result;
}

cb_strview_t cb_sv_chop_right_by_delim(cb_strview_t *sv, char delim) {
    size_t i = sv->count;
    while (i-- && sv->data[i] != delim)
        ;
    cb_strview_t result = cb_sv_from_parts(sv->data + i + 1, sv->count - i - 1);
    sv->count -= (i < sv->count) ? (result.count + 1) : 0;
    return result;
}

cb_strview_t cb_sv_chop_left_by_delim(cb_strview_t *sv, char delim) {
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) i += 1;
    cb_strview_t result = cb_sv_from_parts(sv->data, i);
    sv->count -= i + ((i < sv->count) ? 1 : 0);
    sv->data += i + ((i < sv->count) ? 1 : 0);
    return result;
}

cb_strview_t cb_sv_trim_left(cb_strview_t sv) {
    size_t i = 0;
    while (i < sv.count && CB_IS_SPACE(sv.data[i])) i += 1;
    return cb_sv_from_parts(sv.data + i, sv.count - i);
}

cb_strview_t cb_sv_trim_right(cb_strview_t sv) {
    size_t i = 0;
    while (i < sv.count && CB_IS_SPACE(sv.data[sv.count - 1 - i])) i += 1;
    return cb_sv_from_parts(sv.data, sv.count - i);
}

cb_strview_t cb_sv_trim(cb_strview_t sv) { return cb_sv_trim_right(cb_sv_trim_left(sv)); }
cb_strview_t cb_sv_from_cstr(const char *cstr) { return cb_sv_from_parts(cstr, strlen(cstr)); }
bool         cb_sv_eq(cb_strview_t a, cb_strview_t b) { return (a.count != b.count) ? false : memcmp(a.data, b.data, a.count) == 0; }

cb_target_t *cb_create_target(cb_t *cb, cb_strview_t name, cb_target_type_t type) {
    cb_da_append(cb, ((cb_target_t){.type = type, .name = name}));
    return cb_da_last(cb);
}

// init cb_t returning pointer of the `cb_t`, if error, return `NULL`
// `cb_t` is created in head, it should `cb_deinit` after using it
cb_t *cb_init(int argc, char **argv) {
    (void)argc;
    (void)argv;
    CB_INFO("Initializing CB (C Builder)");
    cb_t *cb = (cb_t *)CB_REALLOC(NULL, sizeof(cb_t));
    return cb;
}
cb_status_t cb_run(cb_t *cb) {
    (void)cb;
    CB_INFO("Running CB (C Builder)");
    return CB_OK;
}
void cb_deinit(cb_t *cb) {
    CB_INFO("Deinitializing CB (C Builder)");
    CB_FREE(cb); 
}

// RETURNS:
//  0 - file does not exists
//  1 - file exists
// -1 - error while checking if file exists. The error is logged
int cb_file_exists(const char *file_path) {
#    if CB_WINDOWS
    // TODO: distinguish between "does not exists" and other errors
    DWORD dwAttrib = GetFileAttributesA(file_path);
    return dwAttrib != INVALID_FILE_ATTRIBUTES;
#    else
    struct stat statbuf;
    if (stat(file_path, &statbuf) < 0) {
        if (errno == ENOENT) return 0;
        CB_ERROR("Could not check if file %s exists: %s", file_path, strerror(errno));
        return -1;
    }
    return 1;
#    endif
}

#    ifdef CB_WINDOWS
struct DIR {
    HANDLE          hFind;
    WIN32_FIND_DATA data;
    struct dirent  *dirent;
};

DIR *opendir(const char *dirpath) {
    assert(dirpath);

    char buffer[MAX_PATH];
    snprintf(buffer, MAX_PATH, "%s\\*", dirpath);

    DIR *dir   = (DIR *)calloc(1, sizeof(DIR));

    dir->hFind = FindFirstFile(buffer, &dir->data);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        // TODO: opendir should set errno accordingly on FindFirstFile fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        goto fail;
    }
    return dir;

fail:
    if (dir) free(dir);
    return NULL;
}

struct dirent *readdir(DIR *dirp) {
    assert(dirp);
    if (dirp->dirent == NULL) {
        dirp->dirent = (struct dirent *)calloc(1, sizeof(struct dirent));
    } else {
        if (!FindNextFile(dirp->hFind, &dirp->data)) {
            if (GetLastError() != ERROR_NO_MORE_FILES) {
                // TODO: readdir should set errno accordingly on FindNextFile fail
                // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
                errno = ENOSYS;
            }
            return NULL;
        }
    }
    memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));
    strncpy(dirp->dirent->d_name, dirp->data.cFileName, sizeof(dirp->dirent->d_name) - 1);
    return dirp->dirent;
}

int closedir(DIR *dirp) {
    assert(dirp);
    if (!FindClose(dirp->hFind)) {
        // TODO: closedir should set errno accordingly on FindClose fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        return -1;
    }
    if (dirp->dirent) free(dirp->dirent);
    free(dirp);
    return 0;
}
#    endif  // CB_WINDOWS

#endif
