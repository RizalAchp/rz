// rz_set.h - simple hash set datastructure implementation in C
//
// To build this, one source file that includes this file do
//      #define RZ_HASH_IMPLEMENTATION
//
// Options: Define RZ_HASH_NO_STDLIB to compile with no stdlib but you must define
//          - `RZ_ASSERT` (optional)
//          - `RZ_REALLOC`
//          - `RZ_FREE`
//
//      Define RZ_HASH_FUNCTION for implement your own hash function
//          function type - `rz_set_hasher_t`
//
// Standard libraries:
//      stdint.h    int types
//      stdlib.h    realloc, free
//      string.h    strlen, memcpy
//
// Credits:
//      Written by Rizal Achmad Pahlevi.

#ifndef __RZ_HASH_H__
#define __RZ_HASH_H__

#if __STDC_VERSION__ <= 201112L
#    error "this  program requires minimum C compiler with __STDC_VERSION__ >= 201112"
#endif

#define RZ_HASH_PPRINT
#define RZ_HASH_IMPLEMENTATION
// #define RZ_HASH_NO_STDLIB

#ifndef RZ_HASH_NO_STDLIB
#    include <stdarg.h>
#    include <stdint.h>
#    include <string.h>

#    ifdef RZ_HASH_PPRINT
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
typedef u64      rz_hash_t;
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
#endif      // RZ_HASH_NO_STDLIB

#ifndef RZ_FNDEF
#    define RZ_FNDEF
#endif  // RZ_FNDEF

typedef enum { RZ_HASH_OK, RZ_HASH_ERR, RZ_HASH_MEMORY_ALLOC_ERR, RZ_HASH_COLLISION_ERR, RZ_HASH_NOT_FOUND_ERR } rz_hash_status_t;

#define RZ_BEGIN(C)                 _Generic((C), rz_hset_t *: rz_hset_begin, rz_hmap_t *: rz_hmap_begin, default: NULL)(C)
#define RZ_END(C)                   _Generic((C), rz_hset_t *: rz_hset_end, rz_hmap_t *: rz_hmap_end, default: NULL)(C)
#define RZ_FOR_EACH(C, IT_TYPE, IT) for (IT_TYPE *IT = RZ_BEGIN(C); IT != RZ_END(C); IT++)

#ifndef RZ_HASH_FUNCTION
#    define RZ_HASH_FUNCTION rz_hash
#endif  // RZ_HASH_FUNCTION

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SET SECTION ///////////////////////////////////////////////////////////////////////////////////
///
typedef struct rz_hset_item_t rz_hset_item_t;
typedef struct rz_hset_t      rz_hset_t;

RZ_FNDEF rz_usize             rz_hset_size(rz_hset_t *set);
RZ_FNDEF int                  rz_hset_is_empty(rz_hset_t *set);
RZ_FNDEF rz_hset_item_t      *rz_hset_begin(rz_hset_t *set);
RZ_FNDEF rz_hset_item_t      *rz_hset_end(rz_hset_t *set);

/// creation and deletion
RZ_FNDEF rz_hset_t rz_hset_create(rz_usize capacity);
RZ_FNDEF void      rz_hset_delete(rz_hset_t *set);

// copy operator for `rz_hset_t` (from 'set_src' to 'set_dst')
RZ_FNDEF rz_hash_status_t rz_hset_copy(rz_hset_t *set_dst, rz_hset_t *set_src);
// move operator for `rz_hset_t` (from 'set_src' to 'set_dst')
RZ_FNDEF rz_hash_status_t rz_hset_move(rz_hset_t *set_dst, rz_hset_t *set_src);
// swap operator for `rz_hset_t` (from 'set_src' to 'set_dst')
RZ_FNDEF rz_hash_status_t rz_hset_swap(rz_hset_t *set_dst, rz_hset_t *set_src);
RZ_FNDEF rz_hash_t        rz_hset_hash_key(rz_hset_t *set, const char *key, rz_usize sz);
RZ_FNDEF rz_hash_status_t rz_hset_remove(rz_hset_t *set, const char *key, rz_usize sz);
RZ_FNDEF rz_hash_status_t rz_hset_insert(rz_hset_t *set, const char *key, rz_usize sz);
RZ_FNDEF rz_hash_status_t rz_hset_contains(rz_hset_t *set, const char *key, rz_usize sz);
#ifndef RZ_HASH_NO_STDLIB
RZ_FNDEF rz_hash_status_t rz_hset_insert_many_impl(rz_hset_t *set, ...);
#endif
RZ_FNDEF rz_hset_item_t *rz_hset_index(rz_hset_t *set, rz_usize idx);
RZ_FNDEF const char     *rz_hset_index_key(rz_hset_t *set, rz_usize idx);
// helper macro for inserting item with sized tipe or cstr
#define rz_hset_remove_cstr(set, key)   (rz_hset_remove(set, key, RZ_STRLEN(key) + 1))
#define rz_hset_insert_cstr(set, key)   (rz_hset_insert(set, key, RZ_STRLEN(key) + 1))
#define rz_hset_inserts_cstr(set, ...)  (rz_hset_insert_many_impl(set, key, __VA_ARGS__, NULL))
#define rz_hset_contains_cstr(set, key) (rz_hset_contains(set, key, RZ_STRLEN(key) + 1))
#define rz_hset_hash_key_cstr(set, key) (rz_hset_hash_key_parts(set, key, RZ_STRLEN(key) + 1))

#ifdef RZ_HASH_PPRINT
RZ_FNDEF void rz_hset_pprint(rz_hset_t *set);
#endif
#ifdef RZ_HASH_NO_STDLIB
RZ_FNDEF rz_usize __rz_strlen(const char *str);
RZ_FNDEF void    *__rz_memcpy(void *dest, const void *src, rz_usize n);
#endif  // RZ_HASH_NO_STDLIB
RZ_FNDEF rz_hash_t rz_hash(const char *key, rz_usize sz);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MAP SECTION ///////////////////////////////////////////////////////////////////////////////////
///
typedef struct rz_hmap_item_t rz_hmap_item_t;
typedef struct rz_hmap_t      rz_hmap_t;

RZ_FNDEF rz_usize             rz_hmap_size(rz_hmap_t *map);
RZ_FNDEF int                  rz_hmap_is_empty(rz_hmap_t *map);
RZ_FNDEF rz_hmap_item_t      *rz_hmap_begin(rz_hmap_t *map);
RZ_FNDEF rz_hmap_item_t      *rz_hmap_end(rz_hmap_t *map);

/// creation and deletion
RZ_FNDEF rz_hmap_t rz_hmap_create(rz_usize capacity);
RZ_FNDEF void      rz_hmap_delete(rz_hmap_t *map);

// copy operator for `rz_hmap_t` (from 'map_src' to 'map_dst')
RZ_FNDEF rz_hash_status_t rz_hmap_copy(rz_hmap_t *map_dst, rz_hmap_t *map_src);
// move operator for `rz_hmap_t` (from 'map_src' to 'map_dst')
RZ_FNDEF rz_hash_status_t rz_hmap_move(rz_hmap_t *map_dst, rz_hmap_t *map_src);
// swap operator for `rz_hmap_t` (from 'map_src' to 'map_dst')
RZ_FNDEF rz_hash_status_t rz_hmap_swap(rz_hmap_t *map_dst, rz_hmap_t *map_src);

RZ_FNDEF rz_hash_t        rz_hmap_hash_key(rz_hmap_t *map, const char *key);
RZ_FNDEF rz_hash_status_t rz_hmap_remove(rz_hmap_t *map, const char *key);
RZ_FNDEF rz_hash_status_t rz_hmap_insert(rz_hmap_t *map, const char *key, const void *value, rz_usize sz_value);
RZ_FNDEF rz_hash_status_t rz_hmap_contains(rz_hmap_t *map, const char *key);
RZ_FNDEF rz_hmap_item_t  *rz_hmap_index(rz_hmap_t *map, rz_usize idx);
RZ_FNDEF const void      *rz_hmap_index_key(rz_hmap_t *map, rz_usize idx, rz_usize *size_out);

#endif  // __RZ_HASH_H__
#ifdef RZ_HASH_IMPLEMENTATION

rz_hash_t rz_hash(const char *key, rz_usize sz) {
    // FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/)
    rz_hash_t h = 14695981039346656037ULL;  // FNV_OFFSET 64 bit
    for (rz_usize i = 0; i < sz; ++i) {
        h = h ^ (key)[i];
        h = h * 1099511628211ULL;  // FNV_PRIME 64 bit
    }
    return h;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SET IMPL SECTION ///////////////////////////////////////////////////////////////////////////////////
///
struct rz_hset_item_t {
    rz_hash_t hash;
    rz_usize  size;
    char     *key;
};
struct rz_hset_t {
    rz_usize        capacity;
    rz_usize        count;
    rz_hset_item_t *table;
};

#ifdef RZ_HASH_NO_STDLIB
rz_usize __rz_strlen(const char *str) {
    char const *pos = str;
    while (*(++pos))
        ;
    return pos - str;
}
void *__rz_memcpy(void *dest, const void *src, rz_usize n) {
    unsigned char       *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (rz_usize i = 0; i < n; i++) d[i] = s[i];
    return dest;
}
#endif
static inline rz_hash_status_t __rz_hset_contains_internal(rz_hset_t *set, rz_hash_t hash, rz_usize sz, rz_usize *idxout) {
    if (rz_hset_is_empty(set)) return RZ_HASH_ERR;
    for (rz_usize i = 0; i < set->count; i++) {
        if (hash == set->table[i].hash && set->table[i].size == sz) {
            if (idxout) *idxout = i;
            return RZ_HASH_OK;
        }
    }
    return RZ_HASH_ERR;
}

static inline rz_hash_status_t __rz_hset_insert_internal(rz_hset_t *set, rz_hset_item_t *item) {
    if (__rz_hset_contains_internal(set, item->hash, item->size, NULL) == RZ_HASH_OK) return RZ_HASH_COLLISION_ERR;
    if (set->count >= set->capacity) {
        set->capacity = set->capacity == 0 ? 32 : set->capacity * 2;
        set->table    = (rz_hset_item_t *)RZ_REALLOC(set->table, set->capacity * sizeof(rz_hset_item_t));
        if (set->table == NULL) return RZ_HASH_MEMORY_ALLOC_ERR;
    }
    set->table[set->count++] = *item;
    return RZ_HASH_OK;
}

rz_usize                 rz_hset_size(rz_hset_t *set) { return (RZ_ASSERT(set), set->count); }
int                      rz_hset_is_empty(rz_hset_t *set) { return (RZ_ASSERT(set), (set->count == 0)); }

RZ_FNDEF rz_hset_item_t *rz_hset_begin(rz_hset_t *set) {
    RZ_ASSERT(set), RZ_ASSERT(set->count > 0 || set->table != NULL);
    return &set->table[0];
}
RZ_FNDEF rz_hset_item_t *rz_hset_end(rz_hset_t *set) {
    RZ_ASSERT(set), RZ_ASSERT(set->count > 0 || set->table != NULL);
    return &set->table[set->count];
}

rz_hset_t rz_hset_create(rz_usize capacity) {
    rz_hset_t set = {.capacity = capacity, .count = 0};
    set.table     = (rz_hset_item_t *)RZ_REALLOC(NULL, capacity * sizeof(rz_hset_item_t));
    RZ_ASSERT(set.table && "RZ_REALLOC return NULL");
    return set;
}

void rz_hset_delete(rz_hset_t *set) {
    if (set->table == NULL) return;
    for (size_t i = 0; i < set->count; i++) {
        RZ_FREE(set->table[i].key);
        set->table[i].key = NULL;
    }
    RZ_FREE(set->table);
    set->table    = NULL;
    set->count    = 0;
    set->capacity = 0;
}

rz_hash_status_t rz_hset_copy(rz_hset_t *set_dst, rz_hset_t *set_src) {
    RZ_ASSERT(set_dst && set_src && "src and dst is should not be NULL");
    for (rz_usize i = 0; i < set_src->count; i++) {
        rz_hset_item_t *key = rz_hset_index(set_src, i);
        if (__rz_hset_insert_internal(set_dst, key) == RZ_HASH_MEMORY_ALLOC_ERR) return RZ_HASH_MEMORY_ALLOC_ERR;
    }
    return RZ_HASH_OK;
}

rz_hash_status_t rz_hset_move(rz_hset_t *set_dst, rz_hset_t *set_src) {
    RZ_ASSERT(set_dst && set_src && "src and dst is should not be NULL");
    rz_hset_copy(set_dst, set_src);
    rz_hset_delete(set_src);
    return RZ_HASH_OK;
}
rz_hash_status_t rz_hset_swap(rz_hset_t *set_dst, rz_hset_t *set_src) {
    RZ_ASSERT(set_dst && set_src && "src and dst is should not be NULL");
    struct rz_hset_t temp = *set_dst;
    *set_dst              = *set_src;
    *set_src              = temp;
    return RZ_HASH_OK;
}

rz_hash_t rz_hset_hash_key(rz_hset_t *set, const char *key, rz_usize sz) {
    rz_hash_t hash = RZ_HASH_FUNCTION(key, sz);
    if (__rz_hset_contains_internal(set, hash, sz, NULL)) return hash;
    return hash;
}

rz_hash_status_t rz_hset_remove(rz_hset_t *set, const char *key, rz_usize sz) {
    if (rz_hset_is_empty(set)) return RZ_HASH_ERR;
    rz_hash_t hash = RZ_HASH_FUNCTION(key, sz);
    size_t    idx  = 0;
    if (__rz_hset_contains_internal(set, hash, sz, &idx) == RZ_HASH_ERR) return RZ_HASH_NOT_FOUND_ERR;
    RZ_FREE(set->table[idx].key);
    for (; idx < (set->count - 1); idx++) set->table[idx] = set->table[idx + 1];
    set->count--;
    return RZ_HASH_OK;
}

rz_hash_status_t rz_hset_insert(rz_hset_t *set, const char *key, rz_usize sz) {
    RZ_ASSERT(set && "`rz_hset_t set` is NULL");
    rz_hset_item_t item = {.size = sz, .hash = RZ_HASH_FUNCTION(key, sz)};
    item.key            = RZ_REALLOC(NULL, sz * sizeof(*item.key));
    RZ_MEMCPY(item.key, key, sz);
    return __rz_hset_insert_internal(set, &item);
}

rz_hash_status_t rz_hset_contains(rz_hset_t *set, const char *key, rz_usize sz) {
    RZ_ASSERT(set && "`rz_hset_t set` is NULL");
    return __rz_hset_contains_internal(set, RZ_HASH_FUNCTION(key, sz), sz, NULL);
}

#ifndef RZ_HASH_NO_STDLIB
rz_hash_status_t rz_hset_insert_many_impl(rz_hset_t *set, ...) {
    va_list args;
    va_start(args, set);
    // const char *arg;
    const char *arg;
    while ((arg = va_arg(args, const char *)) != NULL) rz_hset_insert_cstr(set, arg);
    va_end(args);
    return RZ_HASH_OK;
}
#endif
rz_hset_item_t *rz_hset_index(rz_hset_t *set, rz_usize idx) {
    RZ_ASSERT(set);
    RZ_ASSERT((idx <= (set)->count) && (0 <= idx));
    return &set->table[idx];
}

const char *rz_hset_index_key(rz_hset_t *set, rz_usize idx) {
    RZ_ASSERT(set);
    RZ_ASSERT((idx <= set->count) && (0 <= idx));
    return rz_hset_index(set, idx)->key;
}

#ifdef RZ_HASH_PPRINT
void rz_hset_pprint(rz_hset_t *set) {
    fprintf(stderr, "set = {\n");
    for (rz_usize i = 0; i < set->count; i++) {
        rz_hset_item_t *it = &set->table[i];
        fprintf(stderr, "\t[%zu] : '%*s' - (%zu),\n", i, (int)it->size, (const char *)(it->key), it->hash);
    }
    fprintf(stderr, "{\n");
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MAP SECTION ///////////////////////////////////////////////////////////////////////////////////
///
struct rz_hmap_item_t {
    rz_hash_t hash;
    rz_usize  key_size;
    rz_usize  value_size;
    char     *key;
    void     *value;
};

struct rz_hmap_t {
    rz_hmap_item_t *table;
    rz_usize        count;
    rz_usize        capacity;
};

static inline rz_hash_status_t __rz_hmap_contains_internal(rz_hmap_t *map, rz_hash_t hash, rz_usize sz, rz_usize *idxout) {
    if (rz_hmap_is_empty(map)) return RZ_HASH_ERR;
    for (rz_usize i = 0; i < map->count; i++) {
        if (hash == map->table[i].hash && map->table[i].key_size == sz) {
            if (idxout) *idxout = i;
            return RZ_HASH_OK;
        }
    }
    return RZ_HASH_ERR;
}
static inline rz_hash_status_t __rz_hmap_insert_internal(rz_hmap_t *map, rz_hmap_item_t *item) {
    if (__rz_hmap_contains_internal(map, item->hash, item->key_size, NULL) == RZ_HASH_OK) return RZ_HASH_COLLISION_ERR;
    if (map->count >= map->capacity) {
        map->capacity = map->capacity == 0 ? 32 : map->capacity * 2;
        map->table    = (rz_hmap_item_t *)RZ_REALLOC(map->table, map->capacity * sizeof(rz_hmap_item_t));
        if (map->table == NULL) return RZ_HASH_MEMORY_ALLOC_ERR;
    }
    map->table[map->count++] = *item;
    return RZ_HASH_OK;
}

rz_usize        rz_hmap_size(rz_hmap_t *map) { return (RZ_ASSERT(map), map->count); }
int             rz_hmap_is_empty(rz_hmap_t *map) { return (RZ_ASSERT(map), map->count == 0); }
rz_hmap_item_t *rz_hmap_begin(rz_hmap_t *map) {
    RZ_ASSERT(map), RZ_ASSERT(map->count > 0 || map->table != NULL);
    return &map->table[0];
}
rz_hmap_item_t *rz_hmap_end(rz_hmap_t *map) {
    RZ_ASSERT(map), RZ_ASSERT(map->count > 0 || map->table != NULL);
    return &map->table[map->count];
}

/// creation and deletion
rz_hmap_t rz_hmap_create(rz_usize capacity) {
    rz_hmap_t map = {.capacity = capacity};
    map.table     = (rz_hmap_item_t *)RZ_REALLOC(NULL, capacity * sizeof(rz_hmap_item_t));
    RZ_ASSERT(map.table && "az_hmap_create - realloc returned NULL");
    map.count = 0;
    return map;
}
void rz_hmap_delete(rz_hmap_t *map) {
    if (map->table == NULL) return;
    for (rz_hmap_item_t *it = rz_hmap_begin(map); it != rz_hmap_end(map); it++) {
        RZ_FREE(it->key);
        RZ_FREE(it->value);
        it->key   = NULL;
        it->value = NULL;
    }
    RZ_FREE(map->table);
    map->table    = NULL;
    map->count    = 0;
    map->capacity = 0;
}

rz_hash_status_t rz_hmap_copy(rz_hmap_t *map_dst, rz_hmap_t *map_src) {
    RZ_ASSERT(map_dst && map_src && "src and dst is should not be NULL");
    for (rz_usize i = 0; i < map_src->count; i++) {
        rz_hmap_item_t *key = rz_hmap_index(map_src, i);
        if (__rz_hmap_insert_internal(map_dst, key) == RZ_HASH_MEMORY_ALLOC_ERR) return RZ_HASH_MEMORY_ALLOC_ERR;
    }
    return RZ_HASH_OK;
}
rz_hash_status_t rz_hmap_move(rz_hmap_t *map_dst, rz_hmap_t *map_src) {
    RZ_ASSERT(map_dst && map_src && "src and dst is should not be NULL");
    rz_hmap_copy(map_dst, map_src);
    rz_hmap_delete(map_src);
    return RZ_HASH_OK;
}
rz_hash_status_t rz_hmap_swap(rz_hmap_t *map_dst, rz_hmap_t *map_src) {
    RZ_ASSERT(map_dst && map_src && "src and dst is should not be NULL");
    struct rz_hmap_t temp = *map_dst;
    *map_dst              = *map_src;
    *map_src              = temp;
    return RZ_HASH_OK;
}
rz_hash_t rz_hmap_hash_key(rz_hmap_t *map, const char *key) {
    rz_usize  sz   = RZ_STRLEN(key) + 1;
    rz_hash_t hash = RZ_HASH_FUNCTION(key, sz);
    if (__rz_hmap_contains_internal(map, hash, sz, NULL)) return hash;
    return hash;
}
rz_hash_status_t rz_hmap_remove(rz_hmap_t *map, const char *key) {
    if (rz_hmap_is_empty(map)) return RZ_HASH_ERR;
    rz_usize  sz   = RZ_STRLEN(key) + 1;
    rz_hash_t hash = RZ_HASH_FUNCTION(key, sz);
    size_t    idx  = 0;
    if (__rz_hmap_contains_internal(map, hash, sz, &idx) == RZ_HASH_ERR) return RZ_HASH_NOT_FOUND_ERR;
    RZ_FREE(map->table[idx].key);
    RZ_FREE(map->table[idx].value);
    for (; idx < (map->count - 1); idx++) map->table[idx] = map->table[idx + 1];
    map->count--;
    return RZ_HASH_OK;
}
rz_hash_status_t rz_hmap_insert(rz_hmap_t *map, const char *key, const void *value, rz_usize value_size) {
    RZ_ASSERT(map && "`rz_hmap_t map` is NULL");
    rz_usize       key_size = RZ_STRLEN(key) + 1;
    rz_hmap_item_t item     = {.hash = RZ_HASH_FUNCTION(key, key_size), .key_size = key_size, .value_size = value_size};
    // TODO: value type size
    item.value = RZ_REALLOC(NULL, value_size);
    item.key   = RZ_REALLOC(NULL, key_size);
    RZ_MEMCPY(item.key, key, key_size);
    RZ_MEMCPY(item.value, value, value_size);
    return __rz_hmap_insert_internal(map, &item);
}
rz_hash_status_t rz_hmap_contains(rz_hmap_t *map, const char *key) {
    RZ_ASSERT(map && "`rz_hmap_t map` is NULL");
    rz_usize sz = RZ_STRLEN(key) + 1;
    return __rz_hmap_contains_internal(map, RZ_HASH_FUNCTION(key, sz), sz, NULL);
}
rz_hmap_item_t *rz_hmap_index(rz_hmap_t *map, rz_usize idx) {
    (RZ_ASSERT(map && "`rz_hmap_t map` is NULL"), RZ_ASSERT(idx < map->count));
    return &map->table[idx];
}
const void *rz_hmap_index_key(rz_hmap_t *map, rz_usize idx, rz_usize *out_size) {
    (RZ_ASSERT(map), RZ_ASSERT(idx < map->count));
    rz_hmap_item_t *item = rz_hmap_index(map, idx);
    *out_size            = item->value_size;
    return item->value;
}

#endif  // RZ_HASH_IMPLEMENTATION
