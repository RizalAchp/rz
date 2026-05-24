#include "rz_tests.h"

#include "rz_argparse.h"
#include "rz_strings.h"
#include "rz_time.h"

const RZ_StrView rz__default_run = rz_sv_static("all");

static void       rz__tests_print_diff_bytes(FILE *out, const char *title_expected, const void *a, const char *title_actual, const void *b, size_t maxlen, size_t bytes_per_row,
                                             bool atty);
static void       rz__tests_case_failure(RZ__TestsGlobalCtx *gctx, RZ__TestCaseCtx *ctx, RZ_Duration dur);
static void       rz__tests_case_success(RZ__TestsGlobalCtx *gctx, RZ__TestCaseCtx *ctx, RZ_Duration dur);
static void       rz__tests_case_run(RZ__TestsGlobalCtx *gctx, RZ__TestCaseCtx *ctx);
static inline int rz__tests_score_cmp(void const *a, void const *b);
static bool       rz__tests_finish(RZ__TestsGlobalCtx *ctx);
static bool       rz__tests_run(RZ__TestsGlobalCtx *ctx, RZ_StrView run_arg);
static bool       rz__tests_lists(RZ__TestsGlobalCtx *ctx);
static void       rz__tests_set_atty(RZ__TestsGlobalCtx *ctx);

RZ_DEF bool rz_tests_run(RZ__TestsGlobalCtx *ctx, int argc, char *argv[]) {
    RZ_StrView run_arg  = rz__default_run;
    bool       list_arg = false;

    RZ_Args args        = {.len = argc, .data = argv};
    auto    prog        = rz_args_next(&args);
    RZ_DBG_ASSERT(prog.is_some);

    RZ_Argparse *ap = rz_ap(prog.unwrap.data, .description = "rz tests runner", .version = "1.0");
    rz_ap_pos_sv(ap, "run", &run_arg, .pos = 0, .value_name = "TESTS_NAME", .required = false, .description = "name of tests that wanna to run");
    rz_ap_arg_flag(ap, "list", &list_arg, .long_flag = "list", .short_flag = "l", .required = false, .description = "lists all of the tests available inside the runner");
    if (!rz_ap_parse(ap, argc, argv)) return false;

    rz__tests_set_atty(ctx);

    if (list_arg) return rz__tests_lists(ctx);
    return rz__tests_run(ctx, run_arg);
}

/* Print side-by-side comparison of two byte buffers.
   title_expected/title_actual: labels (e.g., "expected:" "actual:")
   a/alen and b/blen: buffers and lengths.
   bytes_per_row: how many bytes to show per line (commonly 16 or 8).
*/
void rz__tests_print_diff_bytes(FILE *out, const char *title_expected, const void *_a, const char *title_actual, const void *_b, size_t maxlen, size_t bytes_per_row, bool atty) {
    if (!bytes_per_row) bytes_per_row = 16;
    const unsigned char *a = _a;
    const unsigned char *b = _b;

    /* Header */
    // fprintf(out, "%-10s  %-*s  %-*s  "RZ_ENDLINE, "", (int)(bytes_per_row * 3), "hex",
    //        (int)bytes_per_row, "ascii");
    // fprintf(out, "%-10s  %-*s  %-*s  "RZ_ENDLINE RZ_ENDLINE, "", (int)(bytes_per_row * 3),
    // "-----",
    //        (int)bytes_per_row, "-----");

    for (size_t offset = 0; offset < maxlen; offset += bytes_per_row) {
        size_t row_end = offset + bytes_per_row;
        if (row_end > maxlen) row_end = maxlen;

        /* Expected row: hex */
        fprintf(out, "%-10s  ", title_expected);
        for (size_t i = offset; i < row_end; ++i) {
            unsigned char av      = a[i];
            int           differs = (av != b[i]);
            if (differs && atty) fprintf(out, RZ_ANSI_BRIGHT_RED "%02X " RZ_ANSI_RESET, av);
            else fprintf(out, "%02X ", av);
        }

        /* pad if last row shorter */
        if (row_end - offset < bytes_per_row) {
            int pad = (int)(bytes_per_row - (row_end - offset));
            for (int p = 0; p < pad; ++p) fprintf(out, "   ");
        }

        /* ascii for expected */
        fprintf(out, " ");
        for (size_t i = offset; i < row_end; ++i) {
            unsigned char av      = a[i];
            int           differs = (av != b[i]);
            char          ch      = isprint(av) ? (char)av : '.';
            if (differs && atty) fprintf(out, RZ_ANSI_BRIGHT_RED "%c" RZ_ANSI_RESET, ch);
            else fprintf(out, "%c", ch);
        }
        fprintf(out, RZ_ENDLINE);

        /* Actual row: hex */
        fprintf(out, "%-10s  ", title_actual);
        for (size_t i = offset; i < row_end; ++i) {
            unsigned char bv      = b[i];
            int           differs = (bv != a[i]);
            if (differs && atty) fprintf(out, RZ_ANSI_BRIGHT_GREEN "%02X " RZ_ANSI_RESET, bv);
            else fprintf(out, "%02X ", bv);
        }
        if (row_end - offset < bytes_per_row) {
            int pad = (int)(bytes_per_row - (row_end - offset));
            for (int p = 0; p < pad; ++p) fprintf(out, "   ");
        }

        /* ascii for actual */
        fprintf(out, " ");
        for (size_t i = offset; i < row_end; ++i) {
            unsigned char bv      = b[i];
            int           differs = (bv != a[i]);
            char          ch      = isprint(bv) ? (char)bv : '.';
            if (differs && atty) fprintf(out, RZ_ANSI_BRIGHT_GREEN "%c" RZ_ANSI_RESET, ch);
            else fprintf(out, "%c", ch);
        }
        fprintf(out, RZ_ENDLINE RZ_ENDLINE);
    }
}

static void rz__tests_case_failure(RZ__TestsGlobalCtx *gctx, RZ__TestCaseCtx *ctx, RZ_Duration dur) {
    RZ_UNUSED(dur);
    gctx->failure_count++;
    if (gctx->atty) {
        fprintf(gctx->output, RZ_ANSI_BRIGHT_RED "failed" RZ_ANSI_RESET RZ_ENDLINE);
    } else {
        fprintf(gctx->output, "failed" RZ_ENDLINE);
    }
    fprintf(gctx->output, "\tTests Failed:" RZ_ENDLINE);
    rz_foreach(f, &ctx->failures) {
        fprintf(gctx->output, "\t%s%d: %s failed" RZ_ENDLINE, f->loc.file, f->loc.line, f->expr);

        if (f->bytes != 0) {
            rz__tests_print_diff_bytes(gctx->output, "\t expected", f->expected, "\t actual", f->actual, f->bytes, 16, gctx->atty);
        } else {
            fprintf(gctx->output, "\t expected : %s" RZ_ENDLINE, f->expected);
            fprintf(gctx->output, "\t actual   : %s" RZ_ENDLINE, f->actual);
        }
        if (f->msg) { fprintf(gctx->output, "\t message  : %s" RZ_ENDLINE, f->msg); }
    }
}

static void rz__tests_case_success(RZ__TestsGlobalCtx *gctx, RZ__TestCaseCtx *ctx, RZ_Duration dur) {
    RZ_UNUSED(ctx);
    gctx->success_count++;
    rz_f32 secs = rz_duration_as_secs(rz_f32, dur);
    if (gctx->atty) {
        fprintf(gctx->output, RZ_ANSI_BRIGHT_GREEN "success" RZ_ANSI_RESET "(in %.3fsecs)" RZ_ENDLINE, secs);
    } else {
        fprintf(gctx->output, "success (in %.3fsecs)" RZ_ENDLINE, secs);
    }
}

static void rz__tests_case_run(RZ__TestsGlobalCtx *gctx, RZ__TestCaseCtx *ctx) {
    fprintf(gctx->output, "\ttests %s..", ctx->name);
    RZ_InstantTime start   = rz_instant_now();
    bool           success = ctx->func(ctx);
    RZ_Duration    dur     = rz_instant_elapsed(start);
    if (success && rz_arr_is_empty(&ctx->failures)) {
        rz__tests_case_success(gctx, ctx, dur);
    } else {
        rz__tests_case_failure(gctx, ctx, dur);
    }
}

struct RZ__Score {
    rz_usize    score;
    const char *name;
};

static inline int rz__tests_score_cmp(void const *a, void const *b) {
    struct RZ__Score *aa = (struct RZ__Score *)a;
    struct RZ__Score *bb = (struct RZ__Score *)b;
    return aa->score - bb->score;
}

static bool rz__tests_finish(RZ__TestsGlobalCtx *ctx) {
    if (ctx->atty) {
        fprintf(ctx->output,
                "Finnished running " RZ_ANSI_BRIGHT_CYAN "%zu" RZ_ANSI_RESET " tests. [" RZ_ANSI_BRIGHT_GREEN "success: %zu" RZ_ANSI_RESET ", " RZ_ANSI_BRIGHT_RED
                "failure: %zu" RZ_ANSI_RESET "]" RZ_ENDLINE,
                ctx->failure_count + ctx->success_count, ctx->success_count, ctx->failure_count);
    } else {
        fprintf(ctx->output, "Finnished running %zu tests. [success: %zu, failure: %zu]" RZ_ENDLINE, ctx->failure_count + ctx->success_count, ctx->success_count,
                ctx->failure_count);
    }

    fflush(ctx->output);
    RZ_Allocator a = ctx->allocator;
    rz_foreach(c, &ctx->cases) {
        rz_foreach(f, &c->failures) {
            if (f->msg) rz_free(a, f->msg, strlen(f->msg) + 1);
            if (f->bytes != 0) {
                rz_free(a, f->expected, f->bytes);
                rz_free(a, f->actual, f->bytes);
            } else {
                rz_free(a, f->expected, strlen(f->expected) + 1);
                rz_free(a, f->actual, strlen(f->actual) + 1);
            }
        }
        rz_arr_free(&c->failures);
    }
    rz_arr_free(&ctx->cases);

    return (ctx->failure_count == 0);
}

static bool rz__tests_run(RZ__TestsGlobalCtx *ctx, RZ_StrView run_arg) {
    if (ctx->atty) {
        fprintf(ctx->output, "Running tests " RZ_ANSI_BRIGHT_CYAN "'%s'" RZ_ANSI_RESET ".", ctx->file_name);
    } else {
        fprintf(ctx->output, "Running tests '%s'.", ctx->file_name);
    }

    if (!rz_sv_eq(run_arg, rz__default_run)) {
        RZ_TEMP_ALLOCATOR_BLOCK(a, {
            RZ_Array(struct RZ__Score) scores = {.allocator = a};
            rz_arr_reserve(&scores, ctx->cases.len);

            RZ__TestCaseCtx *found = NULL;

            rz_arr_foreach(c, &ctx->cases) {
                if (rz_sv_eq_cstr(run_arg, c->name)) {
                    found = c;
                    break;
                }
                rz_usize score = rz_sv_levenshtein_distance_cstr(run_arg, c->name);
                rz_arr_append(&scores, (struct RZ__Score){score, c->name});
            }

            if (found != NULL) {
                fprintf(ctx->output, "1 tests." RZ_ENDLINE);
                rz__tests_case_run(ctx, found);
                return rz__tests_finish(ctx);
            }
            rz_arr_qsort(&scores, rz__tests_score_cmp);

            fprintf(stderr, "ERROR: Unknown tests case name: " RZ_SVFmt RZ_ENDLINE, RZ_SVArg(run_arg));
            fprintf(stderr, "help: do you mean this?" RZ_ENDLINE);
            rz_usize max_score = RZ_MAX(5, scores.len);
            for (rz_usize i = 0; i < max_score; i++) { fprintf(stderr, "   - %s" RZ_ENDLINE, scores.data[i].name); }
        });
        return false;
    }

    fprintf(ctx->output, "%zu tests." RZ_ENDLINE, ctx->cases.len);
    rz_arr_foreach(c, &ctx->cases) {
        rz__tests_case_run(ctx, c);
    }
    return rz__tests_finish(ctx);
}

static bool rz__tests_lists(RZ__TestsGlobalCtx *ctx) {
    printf("TESTS CASES:" RZ_ENDLINE);
    rz_arr_foreach(c, &ctx->cases) {
        printf("    - %s" RZ_ENDLINE, c->name);
    }
    return true;
}

static void rz__tests_set_atty(RZ__TestsGlobalCtx *ctx) {
#ifdef RZ_OS_WINDOWS
    ctx->atty = false;
    DWORD  mode;
    HANDLE h = (HANDLE)_get_osfhandle(_fileno(ctx->output));
    if (h != INVALID_HANDLE_VALUE) {
        if (GetConsoleMode(h, &mode)) {
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            ctx->atty = true;
            // Now ANSI sequences like \x1b[31m will work
        }
    }
#else
    ctx->atty = isatty(fileno(ctx->output));
#endif
}
