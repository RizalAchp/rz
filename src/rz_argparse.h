#pragma once
#ifndef RZ_ARGPARSE_H
#    define RZ_ARGPARSE_H

#    include "rz_common.h"
#    include "rz_strings.h"

#    if defined(__cplusplus)
extern "C" {
#    endif /* ifndef  defined(__cplusplus) */

typedef RZ_Array(RZ_StrView) RZ_StrViews;
typedef struct RZ_Argparse RZ_Argparse;

typedef RZ_ArrayView(char *) RZ_Args;
typedef RZ_Opt(RZ_StrView) RZ_OptArg;
RZ_DEF bool rz_args_next(RZ_Args *args, RZ_StrView *arg);
RZ_DEF bool rz_args_peek(RZ_Args *args, RZ_StrView *arg);

typedef struct {
    const char *name;
    const char *version;
    const char *description;

    RZ_Allocator allocator;

    rz_usize     subcmd_id;
    RZ_Argparse *subcmd_parent_cmd;
    const char  *subcmd_alias;
    bool         subcmd_default;
} RZ__ArgparseOpt;

RZ_DEF RZ_Argparse *rz_ap_opt(RZ__ArgparseOpt opt);
#    define rz_ap(program_name, ...)                 rz_ap_opt((RZ__ArgparseOpt){.name = program_name __VA_OPT__(, ) __VA_ARGS__})
#    define rz_ap_subcmd(parent_cmd, _name, id, ...) rz_ap_opt((RZ__ArgparseOpt){.subcmd_parent_cmd = parent_cmd, .name = _name, .subcmd_id = id __VA_OPT__(, ) __VA_ARGS__})

RZ_DEF void rz_ap_destroy(RZ_Argparse *ap);
#    define rz_ap_free rz_ap_destroy

RZ_DEF bool rz_ap_parse(RZ_Argparse *ap, int argc, char *argv[]);

RZ_DEF rz_usize rz_ap_subcmd_id(RZ_Argparse *ap);

RZ_DEF void rz_ap_display_help(RZ_Argparse *ap, RZ_Str *buf);
RZ_DEF void rz_ap_print_help(RZ_Argparse *ap);

typedef void (*RZ__ArgparseDisplayCustom)(const void *, RZ_Str *buf);
typedef bool (*RZ__ArgparseParserCustom)(void *value_dest, RZ_StrView value, RZ_Str *error_buf);

RZ_DEF void rz_ap_display_type_cstr(const void *, RZ_Str *buf);
RZ_DEF void rz_ap_display_type_sv(const void *, RZ_Str *buf);
RZ_DEF void rz_ap_display_type_bool(const void *, RZ_Str *buf);
RZ_DEF void rz_ap_display_type_int(const void *, RZ_Str *buf);
RZ_DEF void rz_ap_display_type_uint(const void *, RZ_Str *buf);
RZ_DEF void rz_ap_display_type_float(const void *, RZ_Str *buf);
RZ_DEF void rz_ap_display_type_multi(const void *, RZ_Str *buf);

RZ_DEF bool rz_ap_parse_type_cstr(void *value_dest, RZ_StrView value, RZ_Str *error_buf);
RZ_DEF bool rz_ap_parse_type_sv(void *value_dest, RZ_StrView value, RZ_Str *error_buf);
RZ_DEF bool rz_ap_parse_type_bool(void *value_dest, RZ_StrView value, RZ_Str *error_buf);
RZ_DEF bool rz_ap_parse_type_int(void *value_dest, RZ_StrView value, RZ_Str *error_buf);
RZ_DEF bool rz_ap_parse_type_uint(void *value_dest, RZ_StrView value, RZ_Str *error_buf);
RZ_DEF bool rz_ap_parse_type_float(void *value_dest, RZ_StrView value, RZ_Str *error_buf);
RZ_DEF bool rz_ap_parse_type_multi(void *value_dest, RZ_StrView value, RZ_Str *error_buf);

typedef enum : rz_u8
{
    RZ_AP_CSTR = 0,     // raw cstring (char *)
    RZ_AP_SV,           // StrView
    RZ_AP_BOOL,         // bool or flags
    RZ_AP_INT,          // signed integer   (parsed as i64)
    RZ_AP_UINT,         // unsigned integer (parsed as u64)
    RZ_AP_FLOAT,        // float / double   (parsed as double)
    RZ_AP_MULTI_SV,     // for now, only works on RZ_StrViews ( multiple StrView )
    RZ_AP_CUSTOM,       // custom type  (should provide the custom parser and display)
    RZ_AP_MULTI_CUSTOM, // custom types (should provide the custom parser and display)
} RZ_ArgparseValueType;

typedef struct {
    const char               *id;
    const char               *description;
    const char               *alias;
    const char               *value_name;
    RZ__ArgparseDisplayCustom value_display;
    RZ__ArgparseParserCustom  value_parser;
    void                     *value;
    RZ_ArgparseValueType      type;
    bool                      required;

    const char *long_flag;
    const char *short_flag;
    // if not true. the the parser whould not error.
    // and the value not changed. (defult value)
} RZ_ArgparseArg;

typedef struct {
    const char               *name;
    const char               *description;
    const char               *alias;
    const char               *value_name;
    RZ__ArgparseDisplayCustom value_display;
    RZ__ArgparseParserCustom  value_parser;
    void                     *value;
    RZ_ArgparseValueType      type;
    bool                      required;

    rz_usize pos;
    // if not true. the the parser whould not error.
    // and the value not changed. (defult value)
} RZ_ArgparsePos;

typedef struct {
    union {
        RZ_ArgparsePos as_pos;
        RZ_ArgparseArg as_arg;
    };
    bool pos;
} RZ__ArgparseArgOpt;

RZ_DEF void rz_ap_arg_opt(RZ_Argparse *ap, RZ__ArgparseArgOpt opt);

// clang-format off
#    define rz_ap_arg(ap, _id, _value, ...)          rz_ap_arg_opt(ap, (RZ__ArgparseArgOpt){.pos = false, .as_arg = (RZ_ArgparseArg){.id = _id, .value = _value __VA_OPT__(, ) __VA_ARGS__}})
#    define rz_ap_arg_cstr(ap, id, value, ...)       rz_ap_arg(ap, id, value, .type = RZ_AP_CSTR __VA_OPT__(, ) __VA_ARGS__)
#    define rz_ap_arg_sv(ap, id, value, ...)         rz_ap_arg(ap, id, value, .type = RZ_AP_SV __VA_OPT__(, ) __VA_ARGS__)
#    define rz_ap_arg_flag(ap, id, value, ...)       rz_ap_arg(ap, id, value, .type = RZ_AP_BOOL __VA_OPT__(, ) __VA_ARGS__)
#    define rz_ap_arg_bool                           rz_ap_arg_flag
#    define rz_ap_arg_int(ap, id, value, ...)        rz_ap_arg(ap, id, value, .type = RZ_AP_INT __VA_OPT__(, ) __VA_ARGS__)
#    define rz_ap_arg_uint(ap, id, value, ...)       rz_ap_arg(ap, id, value, .type = RZ_AP_UINT __VA_OPT__(, ) __VA_ARGS__)
#    define rz_ap_arg_float(ap, id, value, ...)      rz_ap_arg(ap, id, value, .type = RZ_AP_FLOAT __VA_OPT__(, ) __VA_ARGS__)
#    define rz_ap_arg_multi(ap, id, value, ...)      rz_ap_arg(ap, id, value, .type = RZ_AP_MULTI_SV __VA_OPT__(, ) __VA_ARGS__)

#    define rz_ap_pos(ap, _id, _value, ...)          rz_ap_arg_opt(ap, (RZ__ArgparseArgOpt){.pos = true, .as_pos = (RZ_ArgparsePos){.name = _id, .value = _value __VA_OPT__(, ) __VA_ARGS__}})
#    define rz_ap_pos_str(ap, name, value, ...)      rz_ap_pos(ap, name, value, .type = RZ_AP_CSTR __VA_OPT__(, ) __VA_ARGS__)
#    define rz_ap_pos_sv(ap, name, value, ...)       rz_ap_pos(ap, name, value, .type = RZ_AP_SV __VA_OPT__(, ) __VA_ARGS__)
#    define rz_ap_pos_int(ap, name, value, ...)      rz_ap_pos(ap, name, value, .type = RZ_AP_INT __VA_OPT__(, ) __VA_ARGS__)
#    define rz_ap_pos_uint(ap, name, value, ...)     rz_ap_pos(ap, name, value, .type = RZ_AP_UINT __VA_OPT__(, ) __VA_ARGS__)
#    define rz_ap_pos_float(ap, name, value, ...)    rz_ap_pos(ap, name, value, .type = RZ_AP_FLOAT __VA_OPT__(, ) __VA_ARGS__)
#    define rz_ap_pos_multi_sv(ap, name, value, ...) rz_ap_pos(ap, name, value, .type = RZ_AP_MULTI_SV __VA_OPT__(, ) __VA_ARGS__)
// clang-format on

#    if defined(__cplusplus)
}
#    endif /* ifndef  defined(__cplusplus) */

#endif     /* end of include guard: RZ_ARGPARSE_H */
