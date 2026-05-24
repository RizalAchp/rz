#include "rz_argparse.h"

#include "rz_common.h"
#include "rz_sprintf.h"

typedef RZ_Array(RZ_Argparse *) RZ_ArgparseSubcmds;
typedef RZ_Array(RZ_ArgparseArg) RZ_ArgparseArgs;
typedef RZ_Array(RZ_ArgparsePos) RZ_ArgparseArgsPos;

struct RZ_Argparse {
    RZ_ArenaAllocator arena;

    RZ_StrViews display_usage;

    RZ_ArgparseArgs    arguments;
    RZ_ArgparseArgsPos pos_arguments;
    RZ_ArgparseSubcmds subcommands;

    RZ_StrView name;
    RZ_StrView alias;
    RZ_StrView version;
    RZ_StrView description;

    RZ_Opt(rz_usize) subcmd_id;
    RZ_Opt(rz_usize) subcmd_selected;
};

static const char *rz__ap_default_value_name(RZ_ArgparseValueType type, const char *n);
static int         rz__ap_count_max_width(RZ_Argparse *ap);
static void        rz__ap_display_help_usages(RZ_Argparse *ap, RZ_Str *buf);
static void        rz__ap_display_help_pos(RZ_Argparse *ap, RZ_ArgparsePos *arg, RZ_Str *buf);
static void        rz__ap_display_help_arg(RZ_Argparse *ap, RZ_ArgparseArg *arg, RZ_Str *buf);
static void        rz__ap_display_help_argparse(RZ_Argparse *ap, RZ_Str *buf);
static bool        rz__ap_parse(RZ_Argparse *ap, RZ_Args *args, RZ_Str *err_buf);
static bool        rz__ap_parse_pos(RZ_Argparse *ap, RZ_ArgparsePos *pos, RZ_StrView value, RZ_Str *err_buf);
static bool        rz__ap_parse_arg(RZ_Argparse *ap, RZ_ArgparseArg *arg, RZ_Args *args, RZ_StrView value, RZ_Str *err_buf);

RZ_DEC bool rz_args_next(RZ_Args *args, RZ_StrView *arg) {
    if (args->len == 0) return false;

    args->len--;
    *arg = rz_sv(*(args->data)++);
    return true;
}

RZ_DEC bool rz_args_peek(RZ_Args *args, RZ_StrView *arg) {
    if (args->len == 0) return false;
    *arg = rz_sv(*(args->data) + 1);
    return true;
}

RZ_DEC RZ_Argparse *rz_ap_opt(RZ__ArgparseOpt opt) {
    RZ_ASSERT((opt.name != NULL), "name for argparse should be not NULL");
    RZ_Argparse      *ap = NULL;
    RZ_Allocator      allocator;
    RZ_ArenaAllocator arena;
    if (opt.subcmd_parent_cmd) {
        arena     = opt.subcmd_parent_cmd->arena;
        allocator = rz_arena_allocator(&opt.subcmd_parent_cmd->arena);

    } else {
        if (!rz_is_allocator(opt.allocator)) opt.allocator = rz_std_allocator();

        arena     = rz_arena(opt.allocator);
        allocator = rz_arena_allocator(&arena);
    }

    ap = rz_calloc(allocator, ap, 1);
    RZ_ASSERT_ALLOCATOR_PTR(ap);

    ap->arena                   = opt.subcmd_parent_cmd->arena;
    ap->name                    = rz_sv(opt.name);
    ap->version                 = rz_sv(opt.version);
    ap->description             = rz_sv(opt.description);

    ap->arguments.allocator     = allocator;
    ap->pos_arguments.allocator = allocator;
    ap->subcommands.allocator   = allocator;
    ap->display_usage.allocator = allocator;

    if (opt.subcmd_parent_cmd) {
        ap->alias = rz_sv(opt.subcmd_alias);

        rz_foreach(it, &opt.subcmd_parent_cmd->subcommands) {
            RZ_ASSERT(opt.subcmd_id != (*it)->subcmd_id.unwrap, "subcmd id should not be equal. try using enum or unique id each subcmd");
        }
        ap->subcmd_id.is_some = true;
        ap->subcmd_id.unwrap  = opt.subcmd_id;
        rz_usize idx          = opt.subcmd_parent_cmd->subcommands.len;
        rz_arr_append(&opt.subcmd_parent_cmd->subcommands, ap);

        if (opt.subcmd_default && !opt.subcmd_parent_cmd->subcmd_selected.is_some) {
            opt.subcmd_parent_cmd->subcmd_selected.is_some = true;
            opt.subcmd_parent_cmd->subcmd_selected.unwrap  = idx;
        }
        rz_foreach(it, &opt.subcmd_parent_cmd->display_usage) {
            rz_arr_append(&ap->display_usage, *it);
        }
    }
    rz_arr_append(&ap->display_usage, ap->name);

    return ap;
}

RZ_DEC bool rz_ap_parse(RZ_Argparse *ap, int argc, char *argv[]) {
    RZ_Args    args = {.len = argc, .data = argv};
    RZ_StrView arg  = {0};
    RZ_ASSERT(rz_args_next(&args, &arg), "argument should always have program name on first argument");

    bool version_flag = false;
    if (!rz_arr_is_empty(&ap->version)) {
        rz_ap_arg_flag(ap, "version", &version_flag, .short_flag = "V", .long_flag = "version", .description = "show version given of the program.");
    }

    bool result = false;
    RZ_TEMP_ALLOCATOR_BLOCK(a, {
        RZ_Str error_buf = {.allocator = a};
        result           = rz__ap_parse(ap, &args, &error_buf);
    });

    if (result && version_flag) {
        fprintf(stdout, RZ_SVFmt " version-" RZ_SVFmt, RZ_SVArg(ap->name), RZ_SVArg(ap->version));
        exit(0);
    }

    return result;
}

RZ_DEC rz_usize rz_ap_subcmd_id(RZ_Argparse *ap) {
    return rz_opt_unwrap(ap->subcmd_selected);
}

RZ_DEC void rz_ap_destroy(RZ_Argparse *ap) {
    RZ_ASSERT_NOT_NULL(ap);
    rz_arena_free(&ap->arena); // just free the arena.
}

RZ_DEC void rz_ap_display_help(RZ_Argparse *ap, RZ_Str *buf) {
    rz__ap_display_help_argparse(ap, buf);
}

RZ_DEC void rz_ap_print_help(RZ_Argparse *ap) {
    RZ_TEMP_ALLOCATOR_BLOCK(a, {
        RZ_Str buf = {.allocator = a};
        rz_ap_display_help(ap, &buf);
        fprintf(stderr, "%.*s", (int)buf.len, buf.data);
    });
}

RZ_DEC void rz_ap_display_type_cstr(const void *v, RZ_Str *buf) {
    const char **val = (const char **)v;
    rz_str_appendf(buf, "%s", *val);
}
RZ_DEC void rz_ap_display_type_sv(const void *v, RZ_Str *buf) {
    const RZ_StrView *val = (const RZ_StrView *)v;
    rz_str_appendf(buf, RZ_SVFmt, RZ_SVArg(*val));
}
RZ_DEC void rz_ap_display_type_bool(const void *v, RZ_Str *buf) {
    const bool *val = (const bool *)v;
    rz_str_appendf(buf, "%s", (*val) ? "true" : "false");
}
RZ_DEC void rz_ap_display_type_int(const void *v, RZ_Str *buf) {
    const rz_i64 *val = (const rz_i64 *)v;
    rz_str_appendf(buf, rz_fmt((rz_i64)0), *val);
}
RZ_DEC void rz_ap_display_type_uint(const void *v, RZ_Str *buf) {
    const rz_u64 *val = (const rz_u64 *)v;
    rz_str_appendf(buf, rz_fmt((rz_u64)0), *val);
}
RZ_DEC void rz_ap_display_type_float(const void *v, RZ_Str *buf) {
    const rz_f64 *val = (const rz_f64 *)v;
    rz_str_appendf(buf, rz_fmt((rz_f64)0), *val);
}
RZ_DEC void rz_ap_display_type_multi(const void *v, RZ_Str *buf) {
    const RZ_StrViews *value = (const RZ_StrViews *)v;
    rz_str_append_cstr(buf, "[");
    rz_foreach_idx(i, it, (RZ_StrViews *)value, {
        if (i != 0) { rz_str_append_cstr(buf, ", "); }
        rz_str_appendf(buf, RZ_SVFmt, RZ_SVArg(*it));
    });
    rz_str_append_cstr(buf, "]");
}

RZ_DEC bool rz_ap_parse_type_cstr(void *value_dest, RZ_StrView value, RZ_Str *error_buf) {
    RZ_UNUSED(error_buf);
    *(char **)value_dest = value.data;
    return true;
}

RZ_DEC bool rz_ap_parse_type_sv(void *value_dest, RZ_StrView value, RZ_Str *error_buf) {
    RZ_UNUSED(error_buf);
    *(RZ_StrView *)value_dest = value;
    return true;
}

RZ_DEF bool rz_ap_parse_type_bool(void *value_dest, RZ_StrView value, RZ_Str *error_buf) {
    RZ_UNUSED(error_buf);
    bool *v = (bool *)value_dest;
    if (rz_sv_case_eq_cstr(value, "true") || rz_sv_eq_cstr(value, "1")) {
        *v = true;
    } else if (rz_sv_case_eq_cstr(value, "false") || rz_sv_eq_cstr(value, "0")) {
        *v = false;
    } else {
        *v = !*v;
    }
    return true;
}

#define rz__ap_helper_parse_primitive(type, ...)                                      \
    do {                                                                              \
        char *end;                                                                    \
        type  v = __VA_ARGS__;                                                        \
        if (end == value.data) { rz_str_append_cstr(error_buf, "malformed number"); } \
        if (rz_errno != 0) {                                                          \
            rz_str_appendf(error_buf, "parse number failed: %s", rz_strerror());      \
            return false;                                                             \
        }                                                                             \
        *(type *)value_dest = v;                                                      \
        return true;                                                                  \
    } while (0)

RZ_DEC bool rz_ap_parse_type_int(void *value_dest, RZ_StrView value, RZ_Str *error_buf) {
    rz__ap_helper_parse_primitive(rz_i64, strtoll(value.data, &end, 10));
}
RZ_DEC bool rz_ap_parse_type_uint(void *value_dest, RZ_StrView value, RZ_Str *error_buf) {
    rz__ap_helper_parse_primitive(rz_u64, strtoull(value.data, &end, 10));
}
RZ_DEC bool rz_ap_parse_type_float(void *value_dest, RZ_StrView value, RZ_Str *error_buf) {
    rz__ap_helper_parse_primitive(rz_f64, strtod(value.data, &end));
}
#undef rz__ap_helper_parse_primitive

RZ_DEC bool rz_ap_parse_type_multi(void *value_dest, RZ_StrView value, RZ_Str *error_buf) {
    RZ_UNUSED(error_buf);
    rz_arr_append((RZ_StrViews *)value_dest, value);
    return true;
}

RZ_DEC void rz_ap_arg_opt(RZ_Argparse *ap, RZ__ArgparseArgOpt opt) {
    RZ_ASSERT_NOT_NULL(ap), RZ_ASSERT_NOT_NULL(opt.as_arg.id), RZ_ASSERT_NOT_NULL(opt.as_arg.value);

    if (!opt.as_arg.value_display) {
        switch (opt.as_arg.type) {
        case RZ_AP_CSTR:
            opt.as_arg.value_display = rz_ap_display_type_cstr;
            break;
        case RZ_AP_SV:
            opt.as_arg.value_display = rz_ap_display_type_sv;
            break;
        case RZ_AP_BOOL:
            opt.as_arg.value_display = rz_ap_display_type_bool;
            break;
        case RZ_AP_INT:
            opt.as_arg.value_display = rz_ap_display_type_int;
            break;
        case RZ_AP_UINT:
            opt.as_arg.value_display = rz_ap_display_type_uint;
            break;
        case RZ_AP_FLOAT:
            opt.as_arg.value_display = rz_ap_display_type_float;
            break;
        case RZ_AP_MULTI_SV:
            opt.as_arg.value_display = rz_ap_display_type_multi;
            break;
        case RZ_AP_CUSTOM:
        case RZ_AP_MULTI_CUSTOM:
        default:
            RZ_ASSERT(opt.as_arg.value_display, "custom types requires value_display function provided.");
            break;
        }
    }
    if (!opt.as_arg.value_parser) {
        switch (opt.as_arg.type) {
        case RZ_AP_CSTR:
            opt.as_arg.value_parser = rz_ap_parse_type_cstr;
            break;
        case RZ_AP_SV:
            opt.as_arg.value_parser = rz_ap_parse_type_sv;
            break;
        case RZ_AP_BOOL:
            opt.as_arg.value_parser = rz_ap_parse_type_bool;
            break;
        case RZ_AP_INT:
            opt.as_arg.value_parser = rz_ap_parse_type_int;
            break;
        case RZ_AP_UINT:
            opt.as_arg.value_parser = rz_ap_parse_type_uint;
            break;
        case RZ_AP_FLOAT:
            opt.as_arg.value_parser = rz_ap_parse_type_float;
            break;
        case RZ_AP_MULTI_SV:
            opt.as_arg.value_parser = rz_ap_parse_type_multi;
            break;
        case RZ_AP_CUSTOM:
        case RZ_AP_MULTI_CUSTOM:
        default:
            RZ_ASSERT(opt.as_arg.value_display, "custom types requires value_parser function provided.");
            break;
        }
    }
    if (!opt.as_arg.value_name) opt.as_arg.value_name = rz__ap_default_value_name(opt.as_arg.type, opt.as_arg.id);

    if (opt.pos) {
        rz_arr_append(&ap->pos_arguments, opt.as_pos);
    } else {
        if (!opt.as_arg.long_flag) opt.as_arg.long_flag = opt.as_arg.id;
        rz_arr_append(&ap->arguments, opt.as_arg);
    }
}

static const char *rz__ap_default_value_name(RZ_ArgparseValueType type, const char *n) {
    switch (type) {
    case RZ_AP_CSTR:
    case RZ_AP_SV:
        return "STRING";
    case RZ_AP_BOOL:
        return NULL;
    case RZ_AP_INT:
        return "SIGNED_NUMBER";
    case RZ_AP_UINT:
        return "UNSIGNED_NUMBER";
    case RZ_AP_FLOAT:
        return "FLOATING_NUMBER";
    case RZ_AP_MULTI_SV:
        return "STRINGS";
    case RZ_AP_CUSTOM:
    case RZ_AP_MULTI_CUSTOM:
    default:
        return n;
        break;
    }
}

static int rz__ap_count_max_width(RZ_Argparse *ap) {
    rz_usize max_width = 0;
    rz_foreach(it, &ap->subcommands) {
        if (max_width < (*it)->name.len) max_width = (*it)->name.len;
    }
    // displayed: <value_name...>
    rz_foreach(it, &ap->pos_arguments) {
        rz_usize w = 2; // 2 for '<' and '>'
        if (it->value_name) {
            w += strlen(it->value_name);
        } else {
            w += strlen(it->name);
        }
        if (it->type == RZ_AP_MULTI_SV) {
            w += 3; // for 3 dot indicating of multiple values
        }
        if (max_width < w) max_width = w;
    }
    // displayed: -short_flag, --long_flag <value_name>
    rz_foreach(it, &ap->arguments) {
        rz_usize w = strlen(it->long_flag) + 2;
        if (it->short_flag) {
            w += strlen(it->short_flag) + 3; // 2 for '-', ',' and ' '
        }
        if (it->value_name) {
            w += strlen(it->value_name) + 3; // 3 for '<', '>', ' '
        }
        if (max_width < w) max_width = w;
    }

    return max_width + 2;
}

static void rz__ap_display_help_usages(RZ_Argparse *ap, RZ_Str *buf) {
    rz_str_appendf(buf, "USAGE: ");
    rz_foreach(it, &ap->display_usage) {
        rz_str_appendf(buf, RZ_SVFmt " ", RZ_SVArg(*it));
    }
    if (!rz_arr_is_empty(&ap->pos_arguments)) { rz_str_append_cstr(buf, "<POSITIONAL_ARGUMENTS> "); }
    if (!rz_arr_is_empty(&ap->arguments)) { rz_str_append_cstr(buf, "<ARGUMENTS> "); }
    if (!rz_arr_is_empty(&ap->subcommands)) { rz_str_append_cstr(buf, "<COMMANDS> "); }
}

static void rz__ap_display_help_pos(RZ_Argparse *ap, RZ_ArgparsePos *arg, RZ_Str *buf) {
    rz_str_appendf(buf, "USAGE: ");
    rz_foreach(it, &ap->display_usage) {
        rz_str_appendf(buf, RZ_SVFmt " ", RZ_SVArg(*it));
    }

    const char *item;
    if (arg->value_name) {
        item = arg->value_name;
    } else {
        item = arg->name;
    }
    rz_str_appendf(buf, "<%s%s>" RZ_ENDLINE, item, (arg->type == RZ_AP_MULTI_SV) ? "..." : "");
    if (arg->description) { rz_str_appendf(buf, "    %s" RZ_ENDLINE, arg->description); }
}

static void rz__ap_display_help_arg(RZ_Argparse *ap, RZ_ArgparseArg *arg, RZ_Str *buf) {
    rz_str_appendf(buf, "USAGE: ");
    rz_foreach(it, &ap->display_usage) {
        rz_str_appendf(buf, RZ_SVFmt " ", RZ_SVArg(*it));
    }
    rz_str_appendf(buf, "--%s ", arg->long_flag);
    if (arg->value_name) rz_str_appendf(buf, "<%s>" RZ_ENDLINE, arg->value_name);
    if (arg->description) rz_str_appendf(buf, "    %s" RZ_ENDLINE, arg->description);
    if (arg->alias) rz_str_appendf(buf, "    [alias: %s]" RZ_ENDLINE, arg->alias);
    if (!arg->required) {
        rz_str_append_cstr(buf, "    [default: ");
        arg->value_display(arg->value, buf);
        rz_str_append_cstr(buf, "]" RZ_ENDLINE);
    }
    rz_str_append_cstr(buf, RZ_ENDLINE);
}

static void rz__ap_display_help_argparse(RZ_Argparse *ap, RZ_Str *buf) {
    RZ_ASSERT(ap && buf);
    int max_width = rz__ap_count_max_width(ap);
    rz__ap_display_help_usages(ap, buf);
    if (!rz_arr_is_empty(&ap->pos_arguments)) {
        rz_str_append_cstr(buf, RZ_ENDLINE RZ_ENDLINE);

        rz_str_append_cstr(buf, "POSITIONAL_ARGUMENTS:" RZ_ENDLINE);
        rz_foreach(it, &ap->pos_arguments) {
            RZ_TEMP_ALLOCATOR_BLOCK(ta, {
                const char *item;
                if (it->value_name) {
                    item = it->value_name;
                } else {
                    item = it->name;
                }
                const char *left  = rz_asprintf(ta, "<%s%s>", item, (it->type == RZ_AP_MULTI_SV) ? "..." : "");
                const char *right = (it->description == NULL) ? "" : it->description;
                rz_str_appendf(buf, "    %-*s%s", max_width, left, right);
            });
            if (it->required) {
                rz_str_append_cstr(buf, " [required]");
            } else {
                rz_str_append_cstr(buf, " [default: ");
                it->value_display(it->value, buf);
                rz_str_append(buf, ']');
            }
            if (it->alias) { rz_str_appendf(buf, " [alias: %s]", it->alias); }
            rz_str_appendf(buf, " [pos: %zu]", it->pos);
            rz_str_append_cstr(buf, RZ_ENDLINE);
        }
    }

    if (!rz_arr_is_empty(&ap->subcommands)) {
        rz_str_append_cstr(buf, RZ_ENDLINE RZ_ENDLINE);

        rz_str_append_cstr(buf, "COMMANDS:" RZ_ENDLINE);
        rz_foreach_idx(i, it, &ap->subcommands, {
            auto sub = *it;
            RZ_TEMP_ALLOCATOR_BLOCK(ta, {
                char *left  = rz_asprintf(ta, RZ_SVFmt, RZ_SVArg(sub->name));
                char *right = rz_arr_is_empty(&sub->description) ? "" : rz_asprintf(ta, RZ_SVFmt, RZ_SVArg(sub->name));
                rz_str_appendf(buf, "    %-*s%s", max_width, left, right);
            });

            if (sub->subcmd_id.is_some) { rz_str_appendf(buf, "[id: %zu]", sub->subcmd_id.unwrap); }
            if (!rz_arr_is_empty(&sub->alias)) { rz_str_appendf(buf, " [alias: " RZ_SVFmt "]", RZ_SVArg(sub->alias)); }
            if (ap->subcmd_selected.is_some && ap->subcmd_selected.unwrap == (*it)->subcmd_id.unwrap) { rz_str_append_cstr(buf, " [default]"); }
            rz_str_append_cstr(buf, RZ_ENDLINE);
        });
    }

    if (!rz_arr_is_empty(&ap->arguments)) {
        rz_str_append_cstr(buf, RZ_ENDLINE RZ_ENDLINE);

        rz_str_append_cstr(buf, "ARGUMENTS:" RZ_ENDLINE);
        rz_foreach(it, &ap->arguments) {
            RZ_TEMP_ALLOCATOR_BLOCK(ta, {
                char *left = "";
                if (it->short_flag && it->value_name) {
                    left = rz_asprintf(ta, "-%s, --%s <%s%s>", it->short_flag, it->long_flag, it->value_name, (it->type == RZ_AP_MULTI_SV) ? "..." : "");
                } else if (it->short_flag) {
                    left = rz_asprintf(ta, "    --%s <%s%s>", it->short_flag, it->long_flag, it->value_name, (it->type == RZ_AP_MULTI_SV) ? "..." : "");
                } else if (it->value_name) {
                    left = rz_asprintf(ta, "-%s, --%s", it->short_flag, it->long_flag);
                } else {
                    left = rz_asprintf(ta, "    --%s", it->long_flag);
                }
                const char *right = (it->description == NULL) ? "" : it->description;
                rz_str_appendf(buf, "    %-*s%s", left, right);
            });

            if (it->required) {
                rz_str_append_cstr(buf, " [required]");
            } else {
                rz_str_append_cstr(buf, " [default: ");
                it->value_display(it->value, buf);
                rz_str_append(buf, ']');
            }
            if (it->alias) { rz_str_appendf(buf, " [alias: %s]", it->alias); }
            rz_str_append_cstr(buf, RZ_ENDLINE);
        }
    }
}

#define rz__ap_print_help_footer(out) fputs("help: try `-h, --help` to display more information" RZ_ENDLINE, out)

static bool rz__ap_parse(RZ_Argparse *ap, RZ_Args *args, RZ_Str *err_buf) {
    RZ_StrView arg         = {0};
    rz_usize   current_pos = 0;

    bool show_help         = false;
    rz_ap_arg_flag(ap, "help", &show_help, .short_flag = "h", .long_flag = "help", .description = "display this help message.");

    RZ_Opt(rz_usize) help_subcmd_id = {.unwrap = -1, .is_some = false};
    RZ_StrView help_subcmd_pos      = rz_sv_empty;
    if (!rz_arr_is_empty(&ap->subcommands)) {
        help_subcmd_id.is_some = true;
        auto subcmd            = rz_ap_subcmd(ap, "help", help_subcmd_id.unwrap, .subcmd_alias = "h");
        rz_ap_pos_sv(subcmd, "subcmd", &help_subcmd_pos, .required = false, .pos = 0, .value_name = "SUBCMD_NAME");
    }

    while (rz_args_next(args, &arg)) {
        if (rz_sv_starts_with_cstr(arg, "-")) {
            RZ_StrView arg_value  = {0};
            arg                   = rz_sv_trim_prefix_char(arg, '-');
            RZ_ArgparseArg *found = NULL;
            rz_foreach(a, &ap->arguments) {
                if (rz_sv_rfind_cstr(arg, "=") != RZ_NOT_FOUND) {
                    RZ_StrView arg_name = {0};
                    if (rz_sv_rsplit_once_char(arg, '=', &arg, &arg_value)) {
                        // if there is an `=`. split an set the left to the value
                        arg = arg_name;
                    }
                }
                if (rz_sv_eq_cstr(arg, a->long_flag) || rz_sv_eq_cstr(arg, a->short_flag)) {
                    found = a;
                    break;
                }
            }
            if (found == NULL) {
                fprintf(stderr, "ERROR: Unknown argument `" RZ_SVFmt "`", RZ_SVArg(arg));
                rz__ap_print_help_footer(stderr);
                return false;
            }

            if (!rz__ap_parse_arg(ap, found, args, arg_value, err_buf)) return false;
            continue;
        }

        RZ_Argparse *subcmd = 0;
        rz_foreach(s, &ap->subcommands) {
            if (rz_sv_eq(arg, (*s)->name)) {
                subcmd = *s;
                break;
            }
        }
        if (subcmd) {
            RZ_DBG_ASSERT(subcmd->subcmd_id.is_some);

            ap->subcmd_selected.is_some = true;
            ap->subcmd_selected.unwrap  = subcmd->subcmd_id.unwrap;
            if (!rz__ap_parse(subcmd, args, err_buf)) return false;
            continue;
        }
        if (current_pos < ap->pos_arguments.len) {
            RZ_ArgparsePos *p = &ap->pos_arguments.data[current_pos++];
            if (!rz__ap_parse_pos(ap, p, arg, err_buf)) return false;
            continue;
        }
        fprintf(stderr, "ERROR: Unknown arguments: `" RZ_SVFmt "`" RZ_ENDLINE, RZ_SVArg(arg));
        rz__ap_print_help_footer(stderr);
        return false;
    }

    RZ_Argparse *to_display = ap;
    if (rz_opt_some_eq(help_subcmd_id, ap->subcmd_selected)) {
        if (!rz_arr_is_empty(&help_subcmd_pos)) {
            rz_foreach(sub, &ap->subcommands) {
                if (rz_sv_eq((*sub)->name, help_subcmd_pos)) {
                    to_display = *sub;
                    break;
                }
            }
            if (to_display == ap) {
                fprintf(stderr, "ERROR: Unknown subcmd argument name `" RZ_SVFmt "` to display for subcmd `help`" RZ_ENDLINE, RZ_SVArg(help_subcmd_pos));
                rz__ap_print_help_footer(stderr);
            }
        }
        show_help = true;
    }
    if (show_help) {
        err_buf->len = 0;
        rz__ap_display_help_argparse(to_display, err_buf);
        fprintf(stdout, RZ_SVFmt RZ_ENDLINE, RZ_SVArg(*err_buf));
        err_buf->len = 0;
        exit(0);
    }

    return true;
}

static bool rz__ap_parse_pos(RZ_Argparse *ap, RZ_ArgparsePos *pos, RZ_StrView value, RZ_Str *err_buf) {
    if (!pos->value_parser(pos->value, value, err_buf)) {
        fprintf(stderr, "ERROR: Failed to parse value for positional argument (%zu)" RZ_ENDLINE, pos->pos);
        fprintf(stderr, "ERROR: " RZ_SVFmt RZ_ENDLINE RZ_ENDLINE, RZ_SVArg(*err_buf));
        err_buf->len = 0;
        rz__ap_display_help_pos(ap, pos, err_buf);
        fprintf(stderr, RZ_SVFmt RZ_ENDLINE RZ_ENDLINE, RZ_SVArg(*err_buf));
        err_buf->len = 0;
        rz__ap_print_help_footer(stderr);
        return false;
    }
    return true;
}
static bool rz__ap_parse_arg(RZ_Argparse *ap, RZ_ArgparseArg *arg, RZ_Args *args, RZ_StrView value, RZ_Str *err_buf) {

    if (arg->type == RZ_AP_BOOL) {
        arg->value_parser(arg->value, value, NULL);
    } else if (rz_arr_is_empty(&value)) {
        if (!rz_args_next(args, &value)) {
            if (arg->short_flag) {
                fprintf(stderr, "ERROR: argument `-%s, --%s` required at least one argument" RZ_ENDLINE, arg->short_flag, arg->long_flag);
            } else {
                fprintf(stderr, "ERROR: argument `--%s` required at least one argument" RZ_ENDLINE, arg->long_flag);
            }
            rz__ap_print_help_footer(stderr);
            return false;
        }
    }

    if (!arg->value_parser(arg->value, value, err_buf)) {
        if (arg->short_flag) {
            fprintf(stderr, "ERROR: failed to parse value for argument `-%s, --%s` " RZ_ENDLINE, arg->short_flag, arg->long_flag);
        } else {
            fprintf(stderr, "ERROR: failed to parse value for argument `--%s` " RZ_ENDLINE, arg->long_flag);
        }
        fprintf(stderr, "ERROR: " RZ_SVFmt RZ_ENDLINE RZ_ENDLINE, RZ_SVArg(*err_buf));
        err_buf->len = 0;
        rz__ap_display_help_arg(ap, arg, err_buf);
        fprintf(stderr, RZ_SVFmt RZ_ENDLINE RZ_ENDLINE, RZ_SVArg(*err_buf));
        err_buf->len = 0;
        rz__ap_print_help_footer(stderr);
        return false;
    }
    return true;
}
