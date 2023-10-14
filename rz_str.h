// rz_set.h - simple hash set datastructure implementation in C
//
// To build this, one source file that includes this file do
//      #define RZ_STR_IMPLEMENTATION
//
// Options: Define RZ_NO_STDLIB to compile with no stdlib but you must define
//          - `RZ_ASSERT` (optional)
//          - `RZ_REALLOC`
//          - `RZ_FREE`
//
//      Define RZ_STR_FUNCTION for implement your own hash function
//          function type - `rz_set_hasher_t`
//
// Standard libraries:
//      stdint.h    int types
//      stdlib.h    realloc, free
//      string.h    strlen, memcpy
//
// Credits:
//      Written by Rizal Achmad Pahlevi.

#ifndef __RZ_STR_H__
#define __RZ_STR_H__

#define RZ_STR_IMPLEMENTATION
// #define RZ_NO_STDLIB

#ifndef RZ_NO_STDLIB
#    include <stdarg.h>
#    include <stdint.h>
#    include <string.h>

#    ifdef RZ_STR_PPRINT
#        include <stdio.h>
#    endif

#    if !defined(RZ_REALLOC) && !defined(RZ_FREE)
#        include <stdlib.h>
#        define RZ_REALLOC realloc
#        define RZ_FREE    free
#    else
#        error "You must define both RZ_REALLOC and RZ_FREE, or neither."
#    endif  // RZ_REALLOC && RZ_FREE
#    ifndef RZ_ASSERT
#        include <assert.h>
#        define RZ_ASSERT assert
#    endif  // RZ_ASSERT
#    ifndef RZ_MEMCPY
#        define RZ_MEMCPY memcpy
#    endif  // RZ_MEMCPY
#    ifndef RZ_STRLEN
#        define RZ_STRLEN strlen
#    endif  // RZ_STRLEN
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;

typedef size_t   usize;
#else
#    define NULL ((void *)0)
#    ifndef RZ_ASSERT
#        define RZ_ASSERT(...) ((void)(__VA_ARGS__))
#    endif  // RZ_ASSERT
#    ifndef RZ_REALLOC
#        define RZ_REALLOC(...) 0
#    endif  // RZ_REALLOC
#    ifndef RZ_FREE
#        define RZ_FREE(...)
#    endif  // RZ_FREE
#    ifndef RZ_MEMCPY
#        error "You must define RZ_MEMCPY, like memcpy from string.h"
#    endif  // RZ_MEMCPY
#    ifndef RZ_STRLEN
#        error "You must define RZ_STRLEN, like strlen from string.h"
#    endif  // RZ_STRLEN
typedef signed char        s8;
typedef signed short int   s16;
typedef signed int         s32;
typedef signed long int    s64;
typedef unsigned char      u8;
typedef unsigned short int u16;
typedef unsigned int       u32;
typedef unsigned long int  u64;
typedef float              f32;
typedef double             f64;
typedef unsigned long      rz_usize;
typedef u64                rz_hash_t;
#endif      // RZ_NO_STDLIB

#ifndef RZ_FNDEF
#    define RZ_FNDEF
#endif  // RZ_FNDEF

#define RZ_IS_SPACE(x)        ((x) == ' ' || (x) == '\t' || (x) == '\n' || (x) == '\r')
#define RZ_GENERIC(tp, sv, s) _Generic((tp), rz_strview_t: sv, rz_str_t: s, default: 0)
typedef enum { RZ_STR_ERR, RZ_STR_OK } rz_str_status_t;

#define rz_str_start_with(s, c) ((s)->items[0] == c)
#define rz_str_end_with(s, c)   ((s)->items[(s)->count - 1] == c)
#define rz_str_trim(s)          RZ_GENERIC(s, rz_sv_trim_left(rz_sv_trim_right(s)), rz_str_trim_left(rz_str_trim_right(s)))
typedef struct rz_str_t     rz_str_t;
typedef struct rz_strview_t rz_strview_t;

struct rz_str_t {
    usize count;
    usize capacity;
    char *items;
};
#define rz_str(cstr)      rz_str_from_parts(cstr, (sizeof(cstr) != sizeof(void *)) ? sizeof(cstr) : RZ_STRLEN(cstr))
#define rz_str_to_sv(str) rz_sv_from_parts((str)->items, (str)->count)
RZ_FNDEF rz_str_t        rz_str_from_parts(const char *data, usize size);
RZ_FNDEF rz_str_status_t rz_str_chop_right_by_delim(rz_str_t *str, rz_strview_t delim, rz_strview_t *chunk);
RZ_FNDEF rz_str_status_t rz_str_chop_left_by_delim(rz_str_t *str, rz_strview_t delim, rz_strview_t *chunk);
RZ_FNDEF rz_str_t        rz_str_trim_left(rz_str_t str);
RZ_FNDEF rz_str_t        rz_str_trim_right(rz_str_t str);
RZ_FNDEF rz_str_t        rz_str_trim_left_by_delim(rz_str_t str, rz_str_t delim);
RZ_FNDEF rz_str_t        rz_str_trim_right_by_delim(rz_str_t str, rz_str_t delim);
RZ_FNDEF const char     *rz_str_as_cstr(rz_str_t *str);

struct rz_strview_t {
    usize       count;
    const char *items;
};

#define rz_sv(cstr) rz_sv_from_parts(cstr, (sizeof(cstr) != sizeof(void *)) ? sizeof(cstr) : RZ_STRLEN(cstr))

RZ_FNDEF rz_strview_t    rz_sv_from_parts(const char *data, usize size);
RZ_FNDEF rz_str_status_t rz_sv_chop_right_by_delim(rz_strview_t *sv, rz_strview_t delim, rz_strview_t *chunk);
RZ_FNDEF rz_str_status_t rz_sv_chop_left_by_delim(rz_strview_t *sv, rz_strview_t delim, rz_strview_t *chunk);
RZ_FNDEF rz_strview_t    rz_sv_trim_left(rz_strview_t sv);
RZ_FNDEF rz_strview_t    rz_sv_trim_right(rz_strview_t sv);
RZ_FNDEF rz_strview_t    rz_sv_trim_left_by_delim(rz_strview_t sv, rz_strview_t delim);
RZ_FNDEF rz_strview_t    rz_sv_trim_right_by_delim(rz_strview_t sv, rz_strview_t delim);

RZ_FNDEF rz_str_status_t rz_sv_eq(rz_strview_t a, rz_strview_t b);
RZ_FNDEF const char     *rz_sv_to_cstr(rz_strview_t sv);

#define rz_sv_trim_by_delim(sv, delim) rz_sv_trim_left_by_delim(rz_sv_trim_right_by_delim(sv, rz_sv(delim)), rz_sv(delim))
#define rz_sv_trim(sv)                 rz_sv_trim_left(rz_sv_trim_right(sv))

#endif  // __RZ_STR_H__
#ifdef RZ_STR_IMPLEMENTATION

// clang-format off
#define __rz_str_zip_iter(astr, bstr, A, B, ...) for (char *A = (char *)(astr)->items; A != &(astr)->items[(astr)->count]; A++) { for (char *B = (char *)(bstr)->items; B != &(bstr)->items[(bstr)->count]; B++) { __VA_ARGS__; } }
#define __rz_str_chop_right_delim(s, delim, chunk)                             \
    size_t i = (s)->count;                                                     \
    __rz_str_zip_iter(s, &delim, a, b, if (*a == *b) {  i -= 1; break; });     \
    if (i < (s)->count) {                                                      \
        if (chunk) {                                                           \
            *chunk = rz_sv_from_parts((s)->items + i + 1, (s)->count - i - 1); \
            (s)->count -= (i < (s)->count) ? (chunk->count + 1) : 0;           \
            return RZ_STR_OK;                                                  \
        }                                                                      \
    }                                                                          \
    return RZ_STR_ERR;
#define __rz_str_chop_left_delim(s, delim, chunk)                          \
    size_t i = 0;                                                          \
    __rz_str_zip_iter( s, &delim, a, b, if (*a == *b) { i += 1; break; }); \
    if (i < (s)->count) {                                                  \
        if (chunk) {                                                       \
            *chunk = rz_sv_from_parts((s)->items, i);                      \
            (s)->count -= i + 1;                                           \
            (s)->items += i + 1;                                           \
            return RZ_STR_OK;                                              \
        }                                                                  \
    }                                                                      \
    return RZ_STR_ERR;
#define __rz_str_trim_left(s)  size_t i = 0; while (i < (s).count && RZ_IS_SPACE((s).items[i])) i += 1; return RZ_GENERIC(s, rz_sv_from_parts, rz_str_from_parts)((s).items + i, (s).count - i);
#define __rz_str_trim_right(s) size_t i = 0; while (i < (s).count && RZ_IS_SPACE((s).items[(s).count - 1 - i])) i += 1; return RZ_GENERIC(s, rz_sv_from_parts, rz_str_from_parts)((s).items, (s).count - i);
#define __rz_str_trim_left_delim(s, delim)                                                \
    size_t i = 0;                                                                         \
    for (size_t x = 0; x < (s).count; x++) {                                              \
        u8 found = 0x0;                                                                   \
        for (size_t y = 0; y < delim.count; y++) found |= (s).items[x] == delim.items[y]; \
        if (found) i += 1;                                                                \
        else break;                                                                       \
    }                                                                                     \
    return RZ_GENERIC(s, rz_sv_from_parts, rz_str_from_parts)((s).items + i, (s).count - i);
#define __rz_str_trim_right_delim(s, delim)                                               \
    size_t i = 0;                                                                         \
    for (size_t x = 0; x < (s).count; x++) {                                              \
        uint8_t found = 0x00;                                                             \
        for (size_t y = 0; y < delim.count; y++) found |= (s).items[x] == delim.items[y]; \
        if (found) i += 1;                                                                \
        else break;                                                                       \
    }                                                                                     \
    return RZ_GENERIC(s, rz_sv_from_parts, rz_str_from_parts)((s).items, (s).count - i);
// clang-format on
#define RZ_STR_ALLOC_CHECK(str)                                                              \
    if ((str)->count >= (str)->capacity) {                                                   \
        (str)->capacity = ((str)->capacity == 0) ? 512 : (str)->capacity * 2;                \
        (str)->items    = RZ_REALLOC((str)->items, (str)->capacity * sizeof(*(str)->items)); \
        RZ_ASSERT((str)->items && "realloc failed, returned NULL");                          \
    }

/// rz_str_t implementation //////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
rz_str_t rz_str_from_parts(const char *data, usize size) {
    rz_str_t str = {.count = size, .capacity = size};
    str.items    = RZ_REALLOC(NULL, size * sizeof(str.items[0]));
    RZ_ASSERT(str.items != NULL && "realloc failed");
    RZ_MEMCPY(str.items, data, size * sizeof(str.items[0]));
    return str;
}
rz_str_status_t rz_str_chop_right_by_delim(rz_str_t *str, rz_strview_t delim, rz_strview_t *chunk) { __rz_str_chop_right_delim(str, delim, chunk); }
rz_str_status_t rz_str_chop_left_by_delim(rz_str_t *str, rz_strview_t delim, rz_strview_t *chunk) { __rz_str_chop_left_delim(str, delim, chunk); }
rz_str_t        rz_str_trim_left(rz_str_t str) { __rz_str_trim_left(str); }
rz_str_t        rz_str_trim_right(rz_str_t str) { __rz_str_trim_right(str); }
rz_str_t        rz_str_trim_left_by_delim(rz_str_t str, rz_str_t delim) { __rz_str_trim_left_delim(str, delim); }
rz_str_t        rz_str_trim_right_by_delim(rz_str_t str, rz_str_t delim) { __rz_str_trim_right_delim(str, delim); }
const char     *rz_str_as_cstr(rz_str_t *str) {
    if (str->count == 0) return NULL;
    RZ_STR_ALLOC_CHECK(str);
    str->items[str->count] = '\0';
    return str->items;
}

/// rz_strview_t implementation //////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
rz_strview_t    rz_sv_from_parts(const char *data, usize size) { return ((rz_strview_t){.items = (const char *)(data), .count = (usize)(size)}); }
rz_str_status_t rz_sv_chop_right_by_delim(rz_strview_t *sv, rz_strview_t delim, rz_strview_t *chunk) { __rz_str_chop_right_delim(sv, delim, chunk); }
rz_str_status_t rz_sv_chop_left_by_delim(rz_strview_t *sv, rz_strview_t delim, rz_strview_t *chunk) { __rz_str_chop_left_delim(sv, delim, chunk); }
rz_strview_t    rz_sv_trim_left(rz_strview_t sv) { __rz_str_trim_left(sv); }
rz_strview_t    rz_sv_trim_right(rz_strview_t sv) { __rz_str_trim_right(sv); }
rz_strview_t    rz_sv_trim_left_by_delim(rz_strview_t sv, rz_strview_t delim) { __rz_str_trim_left_delim(sv, delim); }
rz_strview_t    rz_sv_trim_right_by_delim(rz_strview_t sv, rz_strview_t delim) { __rz_str_trim_right_delim(sv, delim); }
#endif  // RZ_STR_IMPLEMENTATION
