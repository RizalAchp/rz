#pragma once

#ifndef __RZ_LOGGER_H
#    define __RZ_LOGGER_H
#    include "rz_common.h"

#    ifdef __cplusplus
extern "C" {
#    endif

#    ifdef RZ_LOG_DISABLE
///      compile time log disbale
#        define RZ__LOG_DISABLED
#    endif
#    ifndef RZ_LOG_MAX_LEVEL
///      compile time log max level filter
#        define RZ_LOG_MAX_LEVEL RZ_LOG_LEVEL_TRACE
#    endif
#    ifndef RZ_LOG_ENV_FILTER_NAME
#        define RZ_LOG_ENV_FILTER_NAME "RZ_LOG_FILTER"
#    endif

typedef enum : rz_u8
{
    RZ_LOG_LEVEL_OFF = 0,
    RZ_LOG_LEVEL_ERROR,
    RZ_LOG_LEVEL_WARN,
    RZ_LOG_LEVEL_INFO,
    RZ_LOG_LEVEL_DEBUG,
    RZ_LOG_LEVEL_TRACE,
} RZ_LogLevel;

const char *rz_log_level_display(RZ_LogLevel lvl);

typedef struct {
    RZ_LogLevel level;
    const char *tag;
    const char *loc_file;
    int         loc_line;
} RZ_LogRecord;

typedef struct {
    bool (*enable)(void *self, const char *tag, RZ_LogLevel level);
    void (*log)(void *self, RZ_LogRecord record, const char *msg_fmt, va_list msg_args);
    void (*flush)(void *self);
} RZ_LoggerVtable;

typedef struct {
    void                  *self;
    const RZ_LoggerVtable *vtable;
} RZ_Logger;

RZ_DEC void        rz_log_set_max_level(RZ_LogLevel level);
RZ_DEC RZ_LogLevel rz_log_max_level(void);
RZ_DEC bool        rz_log_set_logger(RZ_Logger logger);
RZ_DEC RZ_Logger   rz_log_logger(void);

RZ_DEC bool rz_log_enable(RZ_Logger log, const char *tag, RZ_LogLevel level);
RZ_DEC void rz_log(RZ_Logger log, RZ_LogRecord record, RZ_PRINTF_FMT(const char *msg), ...) RZ_PRINTF_FORMAT(2, 3);
RZ_DEC void rz_log_flush(RZ_Logger log);

// default impementation logger. output to FILE*
// formatted log example: "2026-05-28T12:34:56.789Z LEVEL [TAG] FILE:LINE MESSAGE"
typedef struct {
    // if not provided. default to stdout
    FILE *output;
    // if not provided. default to rz_log_max_level()
    RZ_LogLevel max_level;

    // filter by tag and log_level seperated by bar '|'. [example: "$tag:$log_level|$tag:$log_level"]
    // all tag. use 'all'. [example: "all:info"]
    // log_level is case insensitive. and can be used only the first caracter. [example: "$tag:i,$tag:e"]
    // multiple tag set to one log_level seperated by comma ','. [example: "$tag1,$tag2,$tag3:info"]
    const char *filter;
    // provide filter from environtment variable provided by macro `RZ_LOG_ENV_FILTER_NAME`.
    // the content value of environtment variable follow rules of 'filter' above.
    bool use_env_filter;

    // show tag in the output
    bool show_tag;
    // show location in the output
    bool show_location;
    // show thread id in the output. in ISO8601 timestamp
    bool show_timestamp;
    // show colored output
    bool show_color;

} RZ_LogDefaultOpt;
RZ_DEC bool rz_log_init_default_opt(RZ_LogDefaultOpt opt);
#    define rz_log_init_default(...) rz_log_init_default_impl((RZ_LogDefault){.output = stdout __VA_OPT__(, ) __VA_ARGS__})

#    ifndef RZ__LOG_DISABLED
#        define RZ_LOG(LEVEL, TAG, MSG, ...)                                                                                                                         \
            do {                                                                                                                                                     \
                if (((LEVEL) <= RZ_LOG_MAX_LEVEL) && ((LEVEL) <= rz_log_max_level())) {                                                                              \
                    rz_log(rz_log_logger(), (RZ_LogRecord){.level = LEVEL, .tag = TAG, .loc_file = __FILE__, .loc_line = __LINE__}, MSG __VA_OPT__(, ) __VA_ARGS__); \
                }                                                                                                                                                    \
            } while (0)
#        define RZ_LOG_ENABLE(LEVEL, TAG) (((LEVEL) <= RZ_LOG_MAX_LEVEL) && ((LEVEL) <= rz_log_max_level()) && rz_log_enable(rz_log_logger(), TAG, LEVEL))
#        define RZ_LOG_FLUSH()            rz_log_flush(rz_log_logger())
#    else
#        define RZ_LOG(...)
#        define RZ_LOG_ENABLE(...) false
#        define RZ_LOG_FLUSH()
#    endif

#    define RZ_LOGE(TAG, ...) RZ_LOG(RZ_LOG_LEVEL_ERROR, TAG, __VA_ARGS__)
#    define RZ_LOGW(TAG, ...) RZ_LOG(RZ_LOG_LEVEL_WARN, TAG, __VA_ARGS__)
#    define RZ_LOGI(TAG, ...) RZ_LOG(RZ_LOG_LEVEL_INFO, TAG, __VA_ARGS__)
#    define RZ_LOGD(TAG, ...) RZ_LOG(RZ_LOG_LEVEL_DEBUG, TAG, __VA_ARGS__)
#    define RZ_LOGT(TAG, ...) RZ_LOG(RZ_LOG_LEVEL_TRACE, TAG, __VA_ARGS__)

#    ifdef __cplusplus
} /* extern "C" */
#    endif
#endif /* end of include guard: __RZ_LOGGER_H */
