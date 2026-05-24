#pragma once
#ifndef __RZ_TESTS_H
#    define __RZ_TESTS_H
#    include "rz_collections.h"
#    include "rz_sprintf.h"

/// TODO: implement Per‑test child process. for now this simple test library is just one process.
///       if there any fatal error like segmentation fault, etc. the whole test executable finish.

#    define RZ__TESTS_PREFIX(NAME) rz__tests_case_##NAME
#    if defined(__cplusplus)
#        if defined(RZ_CC_CLANG)
#            define RZ__TESTS_INITIALIZER_BEGIN_DISABLE_WARNINGS _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"")
#            define RZ__TESTS_INITIALIZER_END_DISABLE_WARNINGS   _Pragma("clang diagnostic pop")
#        else
#            define RZ__TESTS_INITIALIZER_BEGIN_DISABLE_WARNINGS
#            define RZ__TESTS_INITIALIZER_END_DISABLE_WARNINGS
#        endif
#        define RZ__TESTS_INITIALIZER(FIXTURE, NAME)                                                                                                      \
            struct RZ__TESTS_PREFIX(register_##FIXTURE##NAME) {                                                                                           \
                RZ__TESTS_PREFIX(register_##FIXTURE##NAME)();                                                                                             \
            };                                                                                                                                            \
            RZ__TESTS_INITIALIZER_BEGIN_DISABLE_WARNINGS static RZ__TESTS_PREFIX(register_##FIXTURE##NAME) RZ__TESTS_PREFIX(register_##FIXTURE##NAME##_g) \
                RZ__TESTS_INITIALIZER_END_DISABLE_WARNINGS;                                                                                               \
            RZ__TESTS_PREFIX(register_##FIXTURE##NAME)::RZ__TESTS_PREFIX(register_##FIXTURE##NAME)()
#    elif defined(RZ_CC_MSVC)
#        if defined(RZ_OS_WINDOWS) && (RZ_ARCH_BITS == 64)
#            define RZ__TESTS_SYMBOL_PREFIX
#        else
#            define RZ__TESTS_SYMBOL_PREFIX "_"
#        endif
#        if defined(RZ_CC_CLANG)
#            define RZ__TESTS_INITIALIZER_BEGIN_DISABLE_WARNINGS _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wmissing-variable-declarations\"")
#            define RZ__TESTS_INITIALIZER_END_DISABLE_WARNINGS   _Pragma("clang diagnostic pop")
#        else
#            define RZ__TESTS_INITIALIZER_BEGIN_DISABLE_WARNINGS
#            define RZ__TESTS_INITIALIZER_END_DISABLE_WARNINGS
#        endif
#        pragma section(".CRT$XCU", read)
#        define RZ__TESTS_INITIALIZER(FIXTURE, NAME)                                                                                                               \
            static void __cdecl RZ__TESTS_PREFIX(register_##FIXTURE##NAME)(void);                                                                                  \
            RZ__TESTS_INITIALIZER_BEGIN_DISABLE_WARNINGS                                                                                                           \
            __pragma(comment(linker, "/include:" RZ__TESTS_SYMBOL_PREFIX RZ_STRINGIFY(RZ__TESTS_PREFIX(register_##FIXTURE##NAME)) "_")) RZ_EXTERN_C                \
                __declspec(allocate(".CRT$XCU")) void(__cdecl * RZ__TESTS_PREFIX(register_##FIXTURE##NAME##_))(void) = RZ__TESTS_PREFIX(register_##FIXTURE##NAME); \
            RZ__TESTS_INITIALIZER_END_DISABLE_WARNINGS                                                                                                             \
            static void __cdecl RZ__TESTS_PREFIX(register_##FIXTURE##NAME)(void)
#    else
#        if defined(__linux__)
#            if defined(__clang__)
#                if __has_warning("-Wreserved-id-macro")
#                    pragma clang diagnostic push
#                    pragma clang diagnostic ignored "-Wreserved-id-macro"
#                endif
#            endif
#            define __STDC_FORMAT_MACROS 1
#            if defined(__clang__)
#                if __has_warning("-Wreserved-id-macro")
#                    pragma clang diagnostic pop
#                endif
#            endif
#        endif
#        define RZ__TESTS_INITIALIZER(FIXTURE, NAME)                                                 \
            static void RZ__TESTS_PREFIX(register_##FIXTURE##NAME)(void) __attribute__(constructor); \
            static void RZ__TESTS_PREFIX(register_##FIXTURE##NAME)(void)
#    endif

#    define RZ_TESTS_FIXTURE(NAME) \
        typedef struct NAME NAME;  \
        struct NAME

#    define RZ_TESTS_SETUP(FIXTURE)    static void rz__tests_case_setup_##FIXTURE(RZ__TestCaseCtx *ctx, FIXTURE *fixture)
#    define RZ_TESTS_TEARDOWN(FIXTURE) static void rz__tests_case_teardown_##FIXTURE(RZ__TestCaseCtx *ctx, FIXTURE *fixture)
#    define RZ_TESTS(FIXTURE, NAME, ...)                                                     \
        static void RZ__TESTS_PREFIX(setup_##FIXTURE)(RZ__TestCaseCtx * ctx, FIXTURE *);     \
        static void RZ__TESTS_PREFIX(teardown_##FIXTURE)(RZ__TestCaseCtx * ctx, FIXTURE *);  \
        static void RZ__TESTS_PREFIX(run_##FIXTURE##NAME)(RZ__TestCaseCtx * ctx, FIXTURE *); \
        static bool RZ__TESTS_PREFIX(FIXTURE##NAME)(RZ__TestCaseCtx * ctx) {                 \
            FIXTURE fixture = {0};                                                           \
            RZ__TESTS_PREFIX(setup_##FIXTURE)(ctx, &fixture);                                \
            if (!rz_arr_is_empty(&ctx->failures)) return false;                              \
            RZ__TESTS_PREFIX(run_##FIXTURE##NAME)(ctx, &fixture);                            \
            if (!rz_arr_is_empty(&ctx->failures)) return false;                              \
            RZ__TESTS_PREFIX(teardown_##FIXTURE)(ctx, &fixture);                             \
            if (!rz_arr_is_empty(&ctx->failures)) return false;                              \
            return true;                                                                     \
        }                                                                                    \
        RZ__TESTS_INITIALIZER(FIXTURE, NAME) {                                               \
            RZ__TestCaseCtx ctx    = {0};                                                    \
            ctx.func               = RZ__TESTS_PREFIX(FIXTURE##NAME);                        \
            ctx.name               = RZ_STRINGIFY(FIXTURE) "::" RZ_STRINGIFY(NAME);          \
            ctx.loc                = rz__test_location();                                    \
            ctx.failures.allocator = rz__tests_global_ctx.allocator;                         \
            rz_arr_append(&rz__tests_global_ctx.cases, ctx);                                 \
        }                                                                                    \
        void RZ__TESTS_PREFIX(run_##FIXTURE##NAME)(RZ__TestCaseCtx * ctx, FIXTURE * fixture)

// clang-format off
                                                 
#    define RZ_TESTS_EXPECT_EQ(X, Y, ...)        RZ__TESTS_CMP(X, Y, ==, xv, yv, xv == yv, #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_EXPECT_NE(X, Y, ...)        RZ__TESTS_CMP(X, Y, !=, xv, yv, xv != yv, #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_EXPECT_LT(X, Y, ...)        RZ__TESTS_CMP(X, Y,  <, xv, yv, xv  < yv, #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_EXPECT_LE(X, Y, ...)        RZ__TESTS_CMP(X, Y, <=, xv, yv, xv <= yv, #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_EXPECT_GT(X, Y, ...)        RZ__TESTS_CMP(X, Y,  >, xv, yv, xv  > yv, #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_EXPECT_GE(X, Y, ...)        RZ__TESTS_CMP(X, Y, >=, xv, yv, xv >= yv, #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)
                                                 
#    define RZ_TESTS_ASSERT_EQ(X, Y, ...)        RZ__TESTS_CMP(X, Y, ==, xv, yv, xv == yv, #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_ASSERT_NE(X, Y, ...)        RZ__TESTS_CMP(X, Y, !=, xv, yv, xv != yv, #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_ASSERT_LT(X, Y, ...)        RZ__TESTS_CMP(X, Y,  <, xv, yv, xv  < yv, #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_ASSERT_LE(X, Y, ...)        RZ__TESTS_CMP(X, Y, <=, xv, yv, xv <= yv, #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_ASSERT_GT(X, Y, ...)        RZ__TESTS_CMP(X, Y,  >, xv, yv, xv  > yv, #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_ASSERT_GE(X, Y, ...)        RZ__TESTS_CMP(X, Y, >=, xv, yv, xv >= yv, #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)
                                                 
#    define RZ_TESTS_EXPECT_STREQ(X, Y, ...)     RZ__TESTS_CMP(X, Y, ==, xv, yv, (rz_strcmp(X, Y) == 0), #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_EXPECT_STRNE(X, Y, ...)     RZ__TESTS_CMP(X, Y, !=, xv, yv, (rz_strcmp(X, Y) != 0), #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)

#    define RZ_TESTS_ASSERT_STREQ(X, Y, ...)     RZ__TESTS_CMP(X, Y, ==, xv, yv, (rz_strcmp(X, Y) == 0), #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_ASSERT_STRNE(X, Y, ...)     RZ__TESTS_CMP(X, Y, !=, xv, yv, (rz_strcmp(X, Y) != 0), #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)

#    define RZ_TESTS_EXPECT_STRNEQ(X, Y, n, ...) RZ__TESTS_CMP(X, Y, ==, xv, yv, (rz_strncmp(X, Y, n) == 0), #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_EXPECT_STRNNE(X, Y, n, ...) RZ__TESTS_CMP(X, Y, !=, xv, yv, (rz_strncmp(X, Y, n) != 0), #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)

#    define RZ_TESTS_ASSERT_STRNEQ(X, Y, n, ...) RZ__TESTS_CMP(X, Y, ==, xv, yv, (rz_strncmp(X, Y, n) == 0), #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_ASSERT_STRNNE(X, Y, n, ...) RZ__TESTS_CMP(X, Y, !=, xv, yv, (rz_strncmp(X, Y, n) != 0), #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)

#    define RZ_TESTS_EXPECT_TRUE(EXPR, ...)      RZ_TESTS_EXPECT_EQ(EXPR, true __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_EXPECT_FALSE(EXPR, ...)     RZ_TESTS_EXPECT_EQ(EXPR, false __VA_OPT__(, ) __VA_ARGS__)
                                                 
#    define RZ_TESTS_ASSERT_TRUE(EXPR, ...)      RZ_TESTS_ASSERT_EQ(EXPR, true __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_ASSERT_FALSE(EXPR, ...)     RZ_TESTS_ASSERT_EQ(EXPR, false __VA_OPT__(, ) __VA_ARGS__)

#    define RZ_TESTS_EXPECT_MEMEQ(X, Y, n, ...)  RZ__TESTS_MEMCMP(X, Y, n, ==, #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_EXPECT_MEMNE(X, Y, n, ...)  RZ__TESTS_MEMCMP(X, Y, n, !=, #X, #Y, false __VA_OPT__(, ) __VA_ARGS__)

#    define RZ_TESTS_ASSERT_MEMEQ(X, Y, n, ...)  RZ__TESTS_MEMCMP(X, Y, n, ==, #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)
#    define RZ_TESTS_ASSERT_MEMNE(X, Y, n, ...)  RZ__TESTS_MEMCMP(X, Y, n, !=, #X, #Y, true __VA_OPT__(, ) __VA_ARGS__)
// clang-format on
//
#    define rz__test_bool_orelse(v) _Generic(v, bool: ((v) ? "true" : "false"), default: v)
#    define RZ__TESTS_CMP(X, Y, op, xv, yv, xy_expr, x_s, y_s, is_assert, ...)                              \
        do {                                                                                                \
            auto xv = X;                                                                                    \
            auto yv = Y;                                                                                    \
            if (!(xy_expr)) {                                                                               \
                RZ__TestCaseFailure _f = {                                                                  \
                    .expr     = "(" x_s " " #op " " y_s ")",                                                \
                    .loc      = rz__test_location(),                                                        \
                    .actual   = rz_asprintf(ctx->failures.allocator, rz_fmt(xv), rz__test_bool_orelse(xv)), \
                    .expected = rz_asprintf(ctx->failures.allocator, rz_fmt(yv), rz__test_bool_orelse(yv)), \
                };                                                                                          \
                __VA_OPT__(_f.msg = rz_asprintf(ctx->failures.allocator, __VA_ARGS__);)                     \
                rz_arr_append(&ctx->failures, _f);                                                          \
                if (is_assert) return;                                                                      \
            }                                                                                               \
        } while (0)

#    define RZ__TESTS_MEMCMP(X, Y, n, op, x_s, y_s, is_assert, ...)                     \
        do {                                                                            \
            auto xv = (x);                                                              \
            auto yv = (y);                                                              \
            if (!(rz_memcmp(xv, yv, n) op 0)) {                                         \
                RZ__TestCaseFailure _f = {                                              \
                    .expr     = "(" x_s " " #op " " y_s ")",                            \
                    .loc      = rz__test_location(),                                    \
                    .actual   = rz_memdup(ctx->failures.allocator, xv, n),              \
                    .expected = rz_memdup(ctx->failures.allocator, xy, n),              \
                    .bytes    = n,                                                      \
                };                                                                      \
                __VA_OPT__(_f.msg = rz_asprintf(ctx->failures.allocator, __VA_ARGS__);) \
                rz_arr_append(&ctx->failures, _f);                                      \
                if (is_assert) return;                                                  \
            }                                                                           \
        } while (0)

#    define RZ_TESTS_MAIN()                                                                \
        RZ__TestsGlobalCtx rz__tests_global_ctx = {0};                                     \
        int                main(int argc, char *argv[]) {                                  \
            rz__tests_global_ctx.file_name = __FILE__;                      \
            rz__tests_global_ctx.output    = stdout;                        \
            rz__tests_global_ctx.allocator = rz_std_allocator();            \
            if (!rz_tests_run(&rz__tests_global_ctx, argc, argv)) return 1; \
            return 0;                                                       \
        }

typedef struct RZ__TestCaseFailure RZ__TestCaseFailure;
typedef struct RZ__TestCaseCtx     RZ__TestCaseCtx;
typedef struct RZ__TestsGlobalCtx  RZ__TestsGlobalCtx;
typedef bool (*RZ__TestFunc)(RZ__TestCaseCtx *ctx);

typedef struct {
    const char *file;
    rz_int      line;
} RZ__TestLocation;
#    define rz__test_location() ((RZ__TestLocation){.file = __FILE__, .line = __LINE__})

struct RZ__TestCaseFailure {
    const char *expr;          // this is static cstr

    const char      *expected; // this is allocated. dont forget free
    const char      *actual;   // this is allocated. dont forget free
    const char      *msg;      // this is allocated. dont forget free
    RZ__TestLocation loc;
    rz_usize         bytes;
};

struct RZ__TestCaseCtx {
    RZ_Array(RZ__TestCaseFailure) failures;

    RZ__TestFunc     func;
    const char      *name;
    RZ__TestLocation loc;
};

struct RZ__TestsGlobalCtx {
    RZ_Array(RZ__TestCaseCtx) cases;
    RZ_Allocator allocator;
    FILE        *output;

    const char *file_name;
    rz_usize    success_count;
    rz_usize    failure_count;
    bool        atty;
};
extern RZ__TestsGlobalCtx rz__tests_global_ctx;

RZ_DEC bool rz_tests_run(RZ__TestsGlobalCtx *ctx, int argc, char *argv[]);

#endif /* end of include guard: __RZ_TESTS_H */
