#pragma once

#ifdef UNIT_TEST

#    include <stdbool.h>
#    include <stdint.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>
#    include <time.h>

#    if defined(__DragonFly__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__)
#        define USE_SYSCTL_FOR_ARGS 1
#        include <sys/sysctl.h>
#        include <sys/types.h>
#        include <unistd.h>  // getpid
#    endif

struct test_file_metadata;

struct test_failure {
    bool        present;
    const char *message;
    const char *file;
    int         line;
    bool        owned;
};

struct test_case_metadata {
    void (*fn)(struct test_case_metadata *, struct test_file_metadata *);
    struct test_failure        failure;
    const char                *name;
    struct test_case_metadata *next;
};

struct test_file_metadata {
    bool                       registered;
    const char                *name;
    struct test_file_metadata *next;
    struct test_case_metadata *tests;
};

struct test_file_metadata __attribute__((weak)) * test_file_head;

#    define SET_FAILURE(_message, _owned)           \
        (metadata->failure = (struct test_failure){ \
             .message = _message,                   \
             .file    = __FILE__,                   \
             .line    = __LINE__,                   \
             .present = true,                       \
             .owned   = _owned,                     \
         })

#    define TEST_EQUAL(a, b)                      \
        do {                                      \
            if ((a) != (b)) {                     \
                SET_FAILURE(#a " != " #b, false); \
                return;                           \
            }                                     \
        } while (0)

#    define TEST_TRUE(a)                               \
        do {                                           \
            if (!(a)) {                                \
                SET_FAILURE(#a " is not true", false); \
                return;                                \
            }                                          \
        } while (0)

#    define TEST_FALSE(a) TEST_TRUE(!(a))

#    define TEST_STREQUAL(a, b)                                    \
        do {                                                       \
            if (strcmp(a, b) != 0) {                               \
                const char *part2 = " != " #b;                     \
                size_t      len   = strlen(a) + strlen(part2) + 3; \
                char       *buf   = malloc(len);                   \
                snprintf(buf, len, "\"%s\"%s", a, part2);          \
                SET_FAILURE(buf, true);                            \
                return;                                            \
            }                                                      \
        } while (0)

#    define TEST_STRNEQUAL(a, b, len)                                  \
        do {                                                           \
            if (strncmp(a, b, len) != 0) {                             \
                const char *part2 = " != " #b;                         \
                size_t      len2  = len + strlen(part2) + 3;           \
                char       *buf   = malloc(len2);                      \
                snprintf(buf, len2, "\"%.*s\"%s", (int)len, a, part2); \
                SET_FAILURE(buf, true);                                \
                return;                                                \
            }                                                          \
        } while (0)

#    define TEST_CASE(_name)                                                                                         \
        static void                      __test_h_##_name(struct test_case_metadata *, struct test_file_metadata *); \
        static struct test_file_metadata __test_h_file;                                                              \
        static struct test_case_metadata __test_h_meta_##_name = {                                                   \
            .name = #_name,                                                                                          \
            .fn   = __test_h_##_name,                                                                                \
        };                                                                                                           \
        static void __attribute__((constructor(101))) __test_h_##_name##_register(void) {                            \
            __test_h_meta_##_name.next = __test_h_file.tests;                                                        \
            __test_h_file.tests        = &__test_h_meta_##_name;                                                     \
            if (!__test_h_file.registered) {                                                                         \
                __test_h_file.name       = __FILE__;                                                                 \
                __test_h_file.next       = test_file_head;                                                           \
                test_file_head           = &__test_h_file;                                                           \
                __test_h_file.registered = true;                                                                     \
            }                                                                                                        \
        }                                                                                                            \
        static void __test_h_##_name(struct test_case_metadata *metadata __attribute__((unused)),                    \
                                     struct test_file_metadata *file_metadata __attribute__((unused)))
#    define SEC_TO_MS(sec)     ((sec)*1000)
#    define SEC_TO_US(sec)     ((sec)*1000000)
#    define SEC_TO_NS(sec)     ((sec)*1000000000)
#    define NS_TO_SEC(ns)      ((ns) / 1000000000)
#    define NS_TO_MS(ns)       ((ns) / 1000000)
#    define NS_TO_US(ns)       ((ns) / 1000)
#    define SEC(sec, nsec)     ((uint64_t)sec + NS_TO_SEC((uint64_t)nsec))
#    define MS(sec, nsec)      (SEC_TO_MS((uint64_t)sec) + NS_TO_MS((uint64_t)nsec))
#    define US(sec, nsec)      (SEC_TO_US((uint64_t)sec) + NS_TO_US((uint64_t)nsec))
#    define NS(sec, nsec)      (SEC_TO_NS((uint64_t)sec) + (uint64_t)nsec)

#    define ANSI_COLOR_RED     "\x1b[31m"
#    define ANSI_COLOR_GREEN   "\x1b[32m"
#    define ANSI_COLOR_YELLOW  "\x1b[33m"
#    define ANSI_COLOR_BLUE    "\x1b[34m"
#    define ANSI_COLOR_MAGENTA "\x1b[35m"
#    define ANSI_COLOR_CYAN    "\x1b[36m"
#    define ANSI_COLOR_RESET   "\x1b[0m"
#    define COLOR(CLR, STR)    ANSI_COLOR_##CLR STR ANSI_COLOR_RESET

extern void __attribute__((weak)) (*test_h_unittest_setup)(void);
/// Run defined tests, return true if all tests succeeds
/// @param[out] tests_run if not NULL, set to whether tests were run
static inline void __attribute__((constructor(102))) run_tests(void) {
    bool should_run = false;
#    ifdef USE_SYSCTL_FOR_ARGS
    int mib[] = {
        CTL_KERN,
#        if defined(__NetBSD__) || defined(__OpenBSD__)
        KERN_PROC_ARGS,
        getpid(),
        KERN_PROC_ARGV,
#        else
        KERN_PROC,
        KERN_PROC_ARGS,
        getpid(),
#        endif
    };
    char  *arg = NULL;
    size_t arglen;
    sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, &arglen, NULL, 0);
    arg = malloc(arglen);
    sysctl(mib, sizeof(mib) / sizeof(mib[0]), arg, &arglen, NULL, 0);
#    else
    FILE *cmdlinef = fopen("/proc/self/cmdline", "r");
    char *arg      = NULL;
    int   arglen;
    fscanf(cmdlinef, "%ms%n", &arg, &arglen);
    fclose(cmdlinef);
#    endif
    for (char *pos = arg; pos < arg + arglen; pos += strlen(pos) + 1) {
        if (strcmp(pos, "--unittest") == 0) {
            should_run = true;
            break;
        }
    }
    free(arg);

    if (!should_run) {
        return;
    }

    if (&test_h_unittest_setup) {
        test_h_unittest_setup();
    }

    struct test_file_metadata *i      = test_file_head;
    int                        failed = 0, success = 0;
    while (i) {
        fprintf(stderr, "Running tests from %s:\n", i->name);

        size_t                     max_len_name = 0;
        struct test_case_metadata *tt           = i->tests;
        while (tt) {
            size_t slen = (strlen(tt->name) + 1);
            if (slen > max_len_name) max_len_name = slen;
            tt = tt->next;
        }
        max_len_name += 2;

        struct test_case_metadata *j = i->tests;
        while (j) {
            struct timespec begin = {0};
            struct timespec end   = {0};
            fprintf(stderr, "\t%s ... ", j->name);
            j->failure.present = false;
            clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
            j->fn(j, i);
            clock_gettime(CLOCK_MONOTONIC_RAW, &end);
            uint64_t st  = (uint64_t)end.tv_sec - (uint64_t)begin.tv_sec;
            uint64_t nst = (uint64_t)end.tv_nsec - (uint64_t)begin.tv_nsec;

            if (j->failure.present) {
                fprintf(stderr, "%*s " COLOR(RED, "failed"), (int)(max_len_name - strlen(j->name)), " ");
                fprintf(stderr, COLOR(YELLOW, " - (%s at %s:%d)") "\n", j->failure.message, j->failure.file, j->failure.line);
                if (j->failure.owned) {
                    free((char *)j->failure.message);
                    j->failure.message = NULL;
                }
                failed++;
            } else {
                uint64_t s  = SEC(st, nst);
                uint64_t ns = NS(st, nst);
                uint64_t us = US(st, nst);
                uint64_t ms = MS(st, nst);

                fprintf(stderr, "%*s " COLOR(GREEN, "passed"), (int)(max_len_name - strlen(j->name)), " ");
                fprintf(stderr, " - (elapsed: %zus:%zums:%zuus:%zuns)\n", s, ms, us, ns);
                success++;
            }
            j = j->next;
        }

        i = i->next;
    }
    int total = failed + success;
    fprintf(stderr, "Test results: passed %d/%d, failed %d/%d\n", success, total, failed, total);
    exit(failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

#else

#    include <stdbool.h>

#    define TEST_CASE(name) static void __attribute__((unused)) __test_h_##name(void)

#    define TEST_EQUAL(a, b) \
        (void)(a);           \
        (void)(b)
#    define TEST_TRUE(a)  (void)(a)
#    define TEST_FALSE(a) (void)(a)
#    define TEST_STREQUAL(a, b) \
        (void)(a);              \
        (void)(b)
#    define TEST_STRNEQUAL(a, b, len) \
        (void)(a);                    \
        (void)(b);                    \
        (void)(len)

#endif
