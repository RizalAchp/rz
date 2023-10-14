// rz_set.h - simple hash set datastructure implementation in C
//
// To build this, one source file that includes this file do
//      #define RZ_ARRAY_IMPLEMENTATION
//
// Options: Define RZ_ARRAY_NO_STDLIB to compile with no stdlib but you must define
//          - `RZ_ASSERT` (optional)
//          - `RZ_REALLOC`
//          - `RZ_FREE`
//
//      Define RZ_ARRAY_FUNCTION for implement your own hash function
//          function type - `rz_set_hasher_t`
//
// Standard libraries:
//      stdint.h    int types
//      stdlib.h    realloc, free
//      string.h    strlen, memcpy
//
// Credits:
//      Written by Rizal Achmad Pahlevi.

#ifndef __RZ_ARRAY_H__
#define __RZ_ARRAY_H__

#if __STDC_VERSION__ <= 201112L
#    error "this  program requires minimum C compiler with __STDC_VERSION__ >= 201112"
#endif

#define RZ_ARRAY_PPRINT
#define RZ_ARRAY_IMPLEMENTATION
// #define RZ_ARRAY_NO_STDLIB

#ifndef RZ_ARRAY_NO_STDLIB
#    include <stdarg.h>
#    include <stdint.h>
#    include <string.h>

#    ifdef RZ_ARRAY_PPRINT
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

typedef size_t   rz_usize;
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
typedef u64                rz_array_t;
#endif      // RZ_ARRAY_NO_STDLIB

#ifndef RZ_FNDEF
#    define RZ_FNDEF
#endif  // RZ_FNDEF

typedef enum { RZ_ARRAY_OK, RZ_ARRAY_ERR, RZ_ARRAY_MEMORY_ALLOC_ERR, RZ_ARRAY_INDEX_ERR, RZ_ARRAY_NOT_FOUND } rz_array_status_t;

#define RZ_BEGIN(C)                 _Generic((C), rz_hset_t *: rz_hset_begin, rz_hmap_t *: rz_hmap_begin, default: NULL)(C)
#define RZ_END(C)                   _Generic((C), rz_hset_t *: rz_hset_end, rz_hmap_t *: rz_hmap_end, default: NULL)(C)
#define RZ_FOR_EACH(C, IT_TYPE, IT) for (IT_TYPE *IT = RZ_BEGIN(C); IT != RZ_END(C); IT++)

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DYNAMIC ARRAY SECTION ///////////////////////////////////////////////////////////////////////////////////
///
#define rz_arr_size(arr)     ((arr)->count)
#define rz_arr_get(arr, idx) ((arr)->items[idx])
#define rz_arr_is_empty(arr) (rz_darr_size(arr) == 0)

#define RZ_ARR_DEFINITION_GENERIC(type)                                                                       \
    typedef struct {                                                                                          \
        rz_usize count;                                                                                       \
        rz_usize capacity;                                                                                    \
        type    *items;                                                                                       \
    } rz_arr_##type##_t;                                                                                      \
    RZ_FNDEF rz_arr_##type##_t rz_arr_##type##_create(rz_usize capacity);                                     \
    RZ_FNDEF void              rz_arr_##type##_delete(rz_arr_##type##_t *arr);                                \
    RZ_FNDEF rz_array_status_t rz_arr_##type##_find(rz_arr_##type##_t *arr, type value, rz_usize *index_out); \
    RZ_FNDEF rz_array_status_t rz_arr_##type##_remove_by_index(rz_arr_##type##_t *arr, rz_usize index);       \
    RZ_FNDEF rz_array_status_t rz_arr_##type##_remove(rz_arr_##type##_t *arr, type value);                    \
    RZ_FNDEF rz_array_status_t rz_arr_##type##_push_front(rz_arr_##type##_t *arr, type value);                \
    RZ_FNDEF rz_array_status_t rz_arr_##type##_push_back(rz_arr_##type##_t *arr, type value);

RZ_ARR_DEFINITION_GENERIC(s8)
RZ_ARR_DEFINITION_GENERIC(s16)
RZ_ARR_DEFINITION_GENERIC(s32)
RZ_ARR_DEFINITION_GENERIC(s64)
RZ_ARR_DEFINITION_GENERIC(u8)
RZ_ARR_DEFINITION_GENERIC(u16)
RZ_ARR_DEFINITION_GENERIC(u32)
RZ_ARR_DEFINITION_GENERIC(u64)
RZ_ARR_DEFINITION_GENERIC(f32)
RZ_ARR_DEFINITION_GENERIC(f64)
RZ_ARR_DEFINITION_GENERIC(rz_usize)

#endif

#ifdef RZ_ARRAY_IMPLEMENTATION

#define RZ_ARRAY_CHECK_SIZE(arr, type, size)                                                \
    if (((arr)->count + size) >= (arr)->capacity) {                                         \
        (arr)->capacity = (arr)->capacity == 0 ? 128 : (arr)->capacity * 2;                 \
        (arr)->items    = (type *)RZ_REALLOC((arr)->items, (arr)->capacity * sizeof(type)); \
        if ((arr)->items == NULL) return RZ_ARRAY_MEMORY_ALLOC_ERR;                         \
    }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DYNAMIC ARRAY IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////
///
#define RZ_ARR_IMPL_GENERIC(type)                                                                      \
    rz_arr_##type##_t rz_arr_##type##_create(rz_usize capacity) {                                      \
        rz_arr_##type##_t arr = {.capacity = capacity};                                                \
        arr.count             = 0;                                                                     \
        arr.items             = RZ_REALLOC(NULL, arr.capacity * sizeof(type));                         \
        RZ_ASSERT(arr.items != NULL && "Buy more memory LOL");                                         \
        return arr;                                                                                    \
    }                                                                                                  \
    void rz_arr_##type##_delete(rz_arr_##type##_t *arr) {                                              \
        if (arr == NULL) return;                                                                       \
        RZ_FREE(arr->items);                                                                           \
        arr->items    = NULL;                                                                          \
        arr->capacity = 0;                                                                             \
        arr->count    = 0;                                                                             \
    }                                                                                                  \
    rz_array_status_t rz_arr_##type##_find(rz_arr_##type##_t *arr, type value, rz_usize *index_out) {  \
        RZ_ASSERT(arr && "rz_arr_##type##_t *arr is NULL");                                            \
        for (rz_usize i = 0; i < arr->count; i++) {                                                    \
            if (arr->items[i] == value && index_out) {                                                 \
                *index_out = i;                                                                        \
                return RZ_ARRAY_OK;                                                                    \
            }                                                                                          \
        }                                                                                              \
        return RZ_ARRAY_NOT_FOUND;                                                                     \
    }                                                                                                  \
    rz_array_status_t rz_arr_##type##_remove_by_index(rz_arr_##type##_t *arr, rz_usize index) {        \
        RZ_ASSERT(arr && "rz_arr_##type##_t *arr is NULL");                                            \
        if (index >= arr->count) return RZ_ARRAY_INDEX_ERR;                                            \
        for (; index < (arr->count - 1); index++) arr->items[index] = arr->items[index + 1];           \
        arr->count--;                                                                                  \
        return RZ_ARRAY_OK;                                                                            \
    }                                                                                                  \
    rz_array_status_t rz_arr_##type##_remove(rz_arr_##type##_t *arr, type value) {                     \
        RZ_ASSERT(arr && "rz_arr_##type##_t *arr is NULL");                                            \
        size_t index = 0;                                                                              \
        if (rz_arr_##type##_find(arr, value, &index) == RZ_ARRAY_NOT_FOUND) return RZ_ARRAY_NOT_FOUND; \
        return rz_arr_##type##_remove_by_index(arr, index);                                            \
    }                                                                                                  \
    rz_array_status_t rz_arr_##type##_push_front(rz_arr_##type##_t *arr, type value) {                 \
        RZ_ASSERT(arr && "rz_arr_##type##_t *arr is NULL");                                            \
        RZ_ARRAY_CHECK_SIZE(arr, type, 2);                                                             \
        for (rz_usize idx = arr->count; idx > 0; idx--) arr->items[idx] = arr->items[idx - 1];         \
        arr->items[0] = value;                                                                         \
        arr->count++;                                                                                  \
        return RZ_ARRAY_OK;                                                                            \
    }                                                                                                  \
    rz_array_status_t rz_arr_##type##_push_back(rz_arr_##type##_t *arr, type value) {                  \
        RZ_ASSERT(arr);                                                                                \
        RZ_ARRAY_CHECK_SIZE(arr, type, 2);                                                             \
        arr->items[arr->count++] = value;                                                              \
        return RZ_ARRAY_OK;                                                                            \
    }

RZ_ARR_IMPL_GENERIC(s8)
RZ_ARR_IMPL_GENERIC(s16)
RZ_ARR_IMPL_GENERIC(s32)
RZ_ARR_IMPL_GENERIC(s64)
RZ_ARR_IMPL_GENERIC(u8)
RZ_ARR_IMPL_GENERIC(u16)
RZ_ARR_IMPL_GENERIC(u32)
RZ_ARR_IMPL_GENERIC(u64)
RZ_ARR_IMPL_GENERIC(f32)
RZ_ARR_IMPL_GENERIC(f64)
RZ_ARR_IMPL_GENERIC(rz_usize)
#endif
