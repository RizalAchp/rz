#ifndef __RZ_SET_H__
#define __RZ_SET_H__

#ifdef RZ_TESTS
#    define RZ_SET_IMPLEMENTATION
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef RZ_ASSERT
#    include <assert.h>
#    define RZ_ASSERT assert
#endif  // defined RZ_ASSERT
#define RZ_ASSERT_ALLOC(PTR) RZ_ASSERT((PTR) && "Buy more RAM lol")
#ifndef RZ_REALLOC
#    define RZ_REALLOC realloc
#endif  // defined RZ_REALLOC
#ifndef RZ_FREE
#    define RZ_FREE free
#endif  // defined RZ_FREE

#ifndef RZ_SET_FNDEF
#    ifdef RZ_SET_IMPLEMENTATION
#        define RZ_SET_FNDEF static
#    else
#        define RZ_SET_FNDEF
#    endif  // defined RZ_SET_IMPLEMENTATION
#endif      // defined RZ_SET_FNDEF

typedef enum {
    RZ_ERR  = 0x0,
    RZ_OK   = 0x1,
    RZ_FAIL = 0x2,
} rz_status_t;

#define RZ_DA_INIT_CAP 256

typedef struct {
    size_t      count;
    const char *data;
} rz_strview_t;

#define rz_sv_size(sv)          ((sv)->count)
#define rz_sv_is_empty(sv)      ((sv)->count == 0)
#define SV_FROM_PART(cstr, len) ((rz_strview_t){.count = len, .data = (const char *)cstr})
#define SV(STATIC_STR)          SV_FROM_PART(STATIC_STR, sizeof(STATIC_STR))

rz_strview_t rz_sv_from_cstr(const char *cstr);
// printf macros for String_View
#ifndef SVFmt
#    define SVFmt "%.*s"
#endif  // SVFmt
#ifndef SVArg
#    define SVArg(sv) (int)(sv).count, (sv).data
#endif  // SVArg

typedef uint64_t rz_hash_t;

typedef rz_hash_t (*rz_set_hasher)(const void *key, size_t sz);
extern rz_set_hasher   rz_hasher_func;

RZ_SET_FNDEF rz_hash_t rz_default_hasher(const void *key, size_t sz);

typedef struct {
    rz_hash_t    hash;
    rz_strview_t key;
} rz_oset_item_t;

typedef struct {
    rz_oset_item_t *items;
    size_t          count;
    size_t          capacity;
} rz_oset_t;
#define rz_oset_size(oset)     ((oset)->count)
#define rz_oset_is_empty(oset) (rz_oset_size(oset) == 0)

RZ_SET_FNDEF rz_oset_t   rz_oset_create(size_t capacity);
RZ_SET_FNDEF rz_status_t rz_oset_insert(rz_oset_t *oset, const rz_strview_t key);
RZ_SET_FNDEF bool        rz_oset_contains(rz_oset_t *oset, const rz_strview_t key);
RZ_SET_FNDEF void        rz_oset_pprint(rz_oset_t *oset);
RZ_SET_FNDEF void        rz_oset_delete(rz_oset_t oset);

// helper macro for param cstr
#define rz_oset_insert_cstr(oset, cstrkey)   rz_oset_insert(oset, rz_sv_from_cstr(cstrkey))
#define rz_oset_contains_cstr(oset, cstrkey) rz_oset_contains(oset, rz_sv_from_cstr(cstrkey))
#define rz_oset_get_byidx(oset, idx)         (RZ_ASSERT((idx <= (oset)->count) && (0 <= idx)), (oset)->items[idx].key)

#endif  // defined __RZ_SET_H__

#ifdef RZ_SET_IMPLEMENTATION

#define RZ_IS_SPACE(x) ((x) == ' ' || (x) == '\t' || (x) == '\n' || (x) == '\r')
rz_set_hasher rz_hasher_func = rz_default_hasher;

rz_strview_t  rz_sv_from_cstr(const char *cstr) {
    char const *pos;
    for (pos = cstr; *pos; ++pos)
        ;
    return SV_FROM_PART(cstr, (size_t)(pos - cstr));
}

rz_hash_t rz_default_hasher(const void *key, size_t sz) {
    // FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/)
    rz_hash_t h = 14695981039346656037ULL;  // FNV_OFFSET 64 bit
    for (size_t i = 0; i < sz; ++i) {
        h = h ^ ((unsigned char *)key)[i];
        h = h * 1099511628211ULL;  // FNV_PRIME 64 bit
    }
    return h;
}
static inline bool __rz_contains_internal(rz_oset_t *oset, const rz_strview_t key, const rz_hash_t hash) {
    if (rz_oset_is_empty(oset)) return false;
    for (size_t i = 0; i < oset->count; i++) {
        rz_oset_item_t *item = &oset->items[i];
        if (key.count == item->key.count && hash == item->hash) return true;
    }
    return false;
}

rz_oset_t rz_oset_create(size_t capacity) {
    rz_oset_t oset = {0};
    oset.items     = (rz_oset_item_t *)RZ_REALLOC(NULL, capacity * sizeof(rz_oset_item_t));
    RZ_ASSERT_ALLOC(oset.items);
    oset.capacity = capacity;
    oset.count    = 0;
    return oset;
}
rz_status_t rz_oset_insert(rz_oset_t *oset, const rz_strview_t key) {
    rz_hash_t hash = rz_hasher_func(key.data, key.count);
    if (__rz_contains_internal(oset, key, hash)) return RZ_ERR;
    if (oset->count >= oset->capacity) {
        size_t last_cap = oset->capacity;
        oset->capacity  = oset->capacity == 0 ? RZ_DA_INIT_CAP : oset->capacity * 2;
        oset->items     = (rz_oset_item_t *)RZ_REALLOC(oset->items, oset->capacity * sizeof(*oset->items));
        RZ_ASSERT_ALLOC(oset->items);
    }
    oset->items[oset->count++] = {.hash = hash, .key = key};
    return RZ_OK;
}
bool rz_oset_contains(rz_oset_t *oset, const rz_strview_t key) { return __rz_contains_internal(oset, key, rz_hasher_func(key.data, key.count)); }
void rz_oset_pprint(rz_oset_t *oset) {
    fprintf(stderr, "ordered_set(oset) = {\n");
    for (size_t i = 0; i < oset->count; i++) fprintf(stderr, "\t[%zu] : ['" SVFmt "' - (%zu)],\n", i, SVArg(oset->items[i].key), oset->items[i].hash);
    fprintf(stderr, "{\n");
}
void rz_oset_delete(rz_oset_t oset) {
    RZ_FREE(oset.items);
    oset.items    = NULL;
    oset.count    = 0;
    oset.capacity = 0;
}

#endif  // defined RZ_SET_IMPLEMENTATION
