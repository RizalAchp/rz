#include "rz_logger.h"

#include "rz_allocator.h"
#include "rz_error.h"
#include "rz_fs.h"
#include "rz_strings.h"

#define RZ_LOG_TAG "rz_logger"

static atomic_uchar RZ__LOG_STATIC_MAX_LEVEL = (RZ_LOG_LEVEL_TRACE);
static atomic_bool  RZ__LOG_INIT             = (false);

static RZ_Logger RZ__LOGGER                  = {0};

const char *rz_log_level_display(RZ_LogLevel lvl);

RZ_DEF void rz_log_set_max_level(RZ_LogLevel level) {
    atomic_store_explicit(&RZ__LOG_STATIC_MAX_LEVEL, level, memory_order_relaxed);
}
RZ_DEF RZ_LogLevel rz_log_max_level(void) {
    auto res = atomic_load_explicit(&RZ__LOG_STATIC_MAX_LEVEL, memory_order_relaxed);
    // debug assert. the RZ__LOG_STATIC_MAX_LEVEL is private.
    // so its guaranteed inrange of RZ_LogLevel. but still on debug assert.
    RZ_DBG_ASSERT(rz_in_range(res, RZ_LOG_LEVEL_OFF, RZ_LOG_LEVEL_TRACE));
    return (RZ_LogLevel)res;
}

RZ_DEF bool rz_log_set_logger(RZ_Logger logger) {
    bool expected = false;
    if (atomic_compare_exchange_strong_explicit(&RZ__LOG_INIT, &expected, true, memory_order_acquire, memory_order_relaxed)) {
        /* we won the initialization race */
        RZ__LOGGER = logger;
        return true;
    }
    // is initialized set the loggeer but return false;
    if (expected) {
        fputs("WARNING: Logger already initialized.\n", stderr);
        RZ__LOGGER = logger;
        return false;
    }
    return false;
}

RZ_DEF RZ_Logger rz_log_logger(void) {
    if (atomic_load_explicit(&RZ__LOG_INIT, memory_order_acquire)) {
        return RZ__LOGGER;
    } else {
        static RZ_Logger nop_logger = {0};
        return nop_logger;
    }
}

static bool rz__logger_default_enable(void *self, const char *target, RZ_LogLevel level);
static void rz__logger_default_log(void *self, RZ_LogRecord record, const char *msg_fmt, va_list msg_args);
static void rz__logger_default_flush(void *self);

RZ_DEF bool rz_log_enable(RZ_Logger log, const char *tag, RZ_LogLevel level) {
    // if the vtable enable is not exists or not provided. its mean always enable.
    if (log.vtable == NULL || log.vtable->enable == NULL) return true;
    return log.vtable->enable(log.self, tag, level);
}

RZ_DEF void rz_log(RZ_Logger log, RZ_LogRecord record, const char *msg, ...) {
    if (log.vtable == NULL) return;
    if (!rz_log_enable(log, record.tag, record.level)) return;

    if (log.vtable->log) {
        va_list arg;
        va_start(arg, msg);
        log.vtable->log(log.self, record, msg, arg);
        va_end(arg);
    }
}

RZ_DEF void rz_log_flush(RZ_Logger log) {
    if (log.vtable == NULL || log.vtable->flush == NULL) return;
    log.vtable->flush(log.self);
}

typedef RZ_Array(struct {
    RZ_StrView  tag;
    RZ_LogLevel level;
}) RZ__LogDefaultFilterMap;

typedef struct {
    RZ__LogDefaultFilterMap filter_map;

    // if not provided. default to stdout
    FILE *output;
    // if not provided. default to rz_log_max_level()
    RZ_LogLevel max_level;

    // show tag in the output
    bool show_tag;
    // show location in the output
    bool show_location;
    // show thread id in the output. in ISO8601 timestamp
    bool show_timestamp;
    // show colored output
    bool show_color;
} RZ__LogDefault;

static bool rz__log_default_filter_map_parse(RZ__LogDefault *l, RZ_StrView filter);

static RZ_LoggerVtable rz__log_default_vtable = {
    .enable = rz__logger_default_enable,
    .log    = rz__logger_default_log,
    .flush  = rz__logger_default_flush,
};

static RZ__LogDefault rz__log_default_logger = {0};

RZ_DEF bool rz_log_init_default_opt(RZ_LogDefaultOpt opt) {
    bool result = true;
    if (opt.output == NULL) opt.output = stdout;
    if (opt.max_level == 0) opt.max_level = rz_log_max_level();

    rz__log_default_logger.output         = opt.output;
    rz__log_default_logger.max_level      = opt.max_level;
    rz__log_default_logger.show_color     = opt.show_color;
    rz__log_default_logger.show_location  = opt.show_location;
    rz__log_default_logger.show_tag       = opt.show_tag;
    rz__log_default_logger.show_timestamp = opt.show_timestamp;

    if (opt.filter) {
        result &= rz__log_default_filter_map_parse(&rz__log_default_logger, rz_sv(opt.filter));
    }
    if (opt.use_env_filter) {
        RZ_Str filter = {.allocator = rz_temp_allocator()};
        if (rz_getenv(RZ_LOG_ENV_FILTER_NAME, &filter)) {
            result &= rz__log_default_filter_map_parse(&rz__log_default_logger, rz_sv_from_str(&filter));
        }
    }

    RZ_Logger log = {.self = &rz__log_default_logger, .vtable = &rz__log_default_vtable};
    result &= rz_log_set_logger(log);

    return result;
}

const char *rz_log_level_display(RZ_LogLevel lvl) {
    switch (lvl) {
    case RZ_LOG_LEVEL_OFF:
        return "OFF";
    case RZ_LOG_LEVEL_ERROR:
        return "ERROR";
    case RZ_LOG_LEVEL_WARN:
        return "WARN";
    case RZ_LOG_LEVEL_INFO:
        return "INFO";
    case RZ_LOG_LEVEL_DEBUG:
        return "DEBUG";
    case RZ_LOG_LEVEL_TRACE:
        return "TRACE";
    default:
        RZ_UNREACHABLE("switch RZ_LogLevel");
    }
}

static bool rz__logger_default_enable(void *self, const char *tag, RZ_LogLevel level) {
    RZ_ASSERT_NOT_NULL(self);
    RZ__LogDefault *l = self;

    if (tag) {
        RZ_StrView   sv_tag = rz_sv(tag);
        RZ_LogLevel *value  = NULL;
        rz_foreach(it, &l->filter_map) {
            if (rz_sv_eq(it->tag, sv_tag)) {
                value = &it->level;
            }
        }
        if (value != NULL) {
            return level <= *value;
        }
    }
    return (level <= l->max_level);
}

static const char *rz_log_level_display_color(RZ_LogLevel lvl) {
    switch (lvl) {
    case RZ_LOG_LEVEL_ERROR:
        return RZ_ANSI_BOLD RZ_ANSI_BRIGHT_RED "ERROR" RZ_ANSI_RESET;
    case RZ_LOG_LEVEL_WARN:
        return RZ_ANSI_BOLD RZ_ANSI_BRIGHT_YELLOW "WARNING" RZ_ANSI_RESET;
    case RZ_LOG_LEVEL_INFO:
        return RZ_ANSI_BOLD RZ_ANSI_BRIGHT_GREEN "INFO" RZ_ANSI_RESET;
    case RZ_LOG_LEVEL_DEBUG:
        return RZ_ANSI_CYAN "DEBUG" RZ_ANSI_RESET;
    case RZ_LOG_LEVEL_TRACE:
        return "TRACE";
        break;

    case RZ_LOG_LEVEL_OFF:
    default:
        RZ_UNREACHABLE("rz_log_level_display_color");
    }
}

static bool rz__log_timestamp(RZ_StrBuilder *sb, bool color) {
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) == 0) return false;

    time_t    sec = ts.tv_sec;
    struct tm tm;
#if defined(RZ_OS_WINDOWS)
    localtime_s(&tm, &sec);
#else
    localtime_r(&sec, &tm);
#endif
    if (color) rz_str_append_cstr(sb, RZ_ANSI_DIM);

    if (strftime(sb->data, sb->capacity, "%Y-%m-%dT%H:%M:%S", &tm) == 0) return false;
    int ms  = (int)(ts.tv_nsec / (1000 * 1000));
    sb->len = strlen(sb->data);
    rz_str_appendf(sb, ".%03d", ms);

    if (color) rz_str_append_cstr(sb, RZ_ANSI_RESET);

    return true;
}

static void rz__logger_default_log(void *self, RZ_LogRecord record, const char *msg_fmt, va_list msg_args) {
    RZ_DBG_ASSERT(self != NULL && msg_fmt != NULL);
    auto temp          = rz_temp_snapshot();

    RZ__LogDefault *l  = self;
    RZ_StrBuilder   sb = {.allocator = rz_temp_allocator()};

    rz_arr_reserve(&sb, 128);
    if (l->show_timestamp) {
        if (!rz__log_timestamp(&sb, l->show_color)) goto end;
    }

    if (l->show_color && rz_isatty(l->output)) {
        fprintf(l->output, "%-7s", rz_log_level_display_color(record.level));
        if (l->show_tag) {
            rz_str_appendf(&sb, "[" RZ_ANSI_BOLD RZ_ANSI_BRIGHT_CYAN "%s" RZ_ANSI_RESET "]", record.tag);
        }
        if (l->show_location) {
            rz_str_appendf(&sb, RZ_ANSI_DIM "%s:%d" RZ_ANSI_RESET, record.loc_file, record.loc_line);
        }
    } else {
        fprintf(l->output, "%-7s", rz_log_level_display(record.level));
        if (l->show_tag) {
            rz_str_appendf(&sb, "[%s]", record.tag);
        }
        if (l->show_location) {
            rz_str_appendf(&sb, "%s:%d", record.loc_file, record.loc_line);
        }
    }

    rz_str_appendvf(&sb, msg_fmt, msg_args);
    rz_str_append_cstr(&sb, RZ_ENDLINE);
    rz_str_append_null(&sb);

    fputs(sb.data, l->output);

end:
    rz_temp_rewind(temp);
}

static void rz__logger_default_flush(void *self) {
    RZ_DBG_ASSERT_NOT_NULL(self);
    fflush(((RZ__LogDefault *)self)->output);
}

static bool rz__log_level_parse(RZ_StrView s, RZ_LogLevel *level) {
    RZ_ASSERT_NOT_NULL(level);
    if (rz_sv_case_eq(s, rz_sv_static("o")) || rz_sv_case_eq(s, rz_sv_static("off"))) {
        *level = RZ_LOG_LEVEL_OFF;
    } else if (rz_sv_case_eq(s, rz_sv_static("e")) || rz_sv_case_eq(s, rz_sv_static("err")) || rz_sv_case_eq(s, rz_sv_static("error"))) {
        *level = RZ_LOG_LEVEL_ERROR;
    } else if (rz_sv_case_eq(s, rz_sv_static("w")) || rz_sv_case_eq(s, rz_sv_static("warn")) || rz_sv_case_eq(s, rz_sv_static("warning"))) {
        *level = RZ_LOG_LEVEL_WARN;
    } else if (rz_sv_case_eq(s, rz_sv_static("i")) || rz_sv_case_eq(s, rz_sv_static("info"))) {
        *level = RZ_LOG_LEVEL_INFO;
    } else if (rz_sv_case_eq(s, rz_sv_static("d")) || rz_sv_case_eq(s, rz_sv_static("debug"))) {
        *level = RZ_LOG_LEVEL_DEBUG;
    } else if (rz_sv_case_eq(s, rz_sv_static("t")) || rz_sv_case_eq(s, rz_sv_static("trace"))) {
        *level = RZ_LOG_LEVEL_TRACE;
    } else {
        RZ_ERROR(RZ_LOG_TAG, "failed to parse log_level. required o|off,e|error,w|warning,i|info,d|debug,t|trace. but got: " RZ_SVFmt, RZ_SVArg(s));
        return false;
    }
    return true;
}

static bool rz__log_default_filter_map_parse(RZ__LogDefault *l, RZ_StrView filter) {
    RZ_StrView key_value = {0};
    RZ_StrView key       = {0};
    RZ_StrView value     = {0};

    RZ_StrView k         = {0};

    RZ_LogLevel level;
    while (rz_sv_split_char(&filter, '|', &key_value)) {
        key_value = rz_sv_trim(key_value);
        if (rz_sv_is_empty(key_value)) continue;

        rz_usize colon = rz_sv_find_char(key_value, ':');
        if (colon == RZ_NOT_FOUND) {
            RZ_ERROR(RZ_LOG_TAG, "failed to parse log filter: `" RZ_SVFmt "`. required ':' between tag and log_level [example: $tag:$log_level]. but got none",
                     RZ_SVArg(key_value));
            return false;
        }
        rz_arr_split_at(&key_value, colon, &key, &value);
        key   = rz_sv_trim(key);
        value = rz_sv_trim(value);
        if (rz_sv_is_empty(key) || rz_sv_is_empty(value)) {
            RZ_ERROR(RZ_LOG_TAG, "failed to parse log filter: `" RZ_SVFmt "`. required key and value on key:value is not empty", RZ_SVArg(key_value));
            return false;
        }

        if (!rz__log_level_parse(value, &level)) return false;
        while (rz_sv_split_char(&key, ',', &k)) {
            k = rz_sv_trim(k);
            if (rz_sv_is_empty(k)) continue;
            if (rz_sv_eq(key, rz_sv_static("all"))) {
                l->max_level = level;
                continue;
            }

            bool found = false;
            rz_foreach(it, &l->filter_map) {
                if (rz_sv_eq(it->tag, k)) {
                    it->level = level;
                    found     = true;
                }
            }
            if (!found) {
                rz_arr_append(&l->filter_map, (RZ_TYPEOF(*l->filter_map.data)){k, level});
            }
        }
    }
    return true;
}
