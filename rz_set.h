#ifndef __RZ_SET_H__
#define __RZ_SET_H__

#define RZ_SET_IMPLEMENTATION

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

#ifndef RZ_FNDEF
#    ifdef RZ_SET_IMPLEMENTATION
#        define RZ_FNDEF static
#    else
#        define RZ_FNDEF
#    endif  // defined RZ_SET_IMPLEMENTATION
#endif      // defined RZ_FNDEF

#define RZ_DA_INIT_CAP 256

typedef enum { RZ_ERR, RZ_OK, RZ_FAIL } rz_status_t;

typedef uint64_t rz_hash_t;

typedef rz_hash_t (*rz_set_hasher)(const void *key, size_t sz);
extern rz_set_hasher rz_hasher_func;

RZ_FNDEF rz_hash_t   rz_default_hasher(const void *key, size_t sz);

typedef struct {
    rz_hash_t hash;
    void     *key;
} rz_oset_item_t;

typedef struct {
    size_t          key_size;
    size_t          capacity;
    size_t          count;
    rz_oset_item_t *items;
} rz_oset_t;
#define rz_oset_size(oset)     ((oset)->count)
#define rz_oset_is_empty(oset) (rz_oset_size(oset) == 0)

RZ_FNDEF rz_oset_t   rz_oset_create(size_t capacity);
RZ_FNDEF rz_status_t rz_oset_insert(rz_oset_t *oset, const void *key, int sz);
RZ_FNDEF bool        rz_oset_contains(rz_oset_t *oset, const void *key);
RZ_FNDEF void        rz_oset_pprint(rz_oset_t *oset);
RZ_FNDEF void        rz_oset_delete(rz_oset_t oset);

// helper macro for param cstr
#define rz_oset_insert_cstr(oset, cstrkey)   rz_oset_insert(oset, rz_sv_from_cstr(cstrkey))
#define rz_oset_contains_cstr(oset, cstrkey) rz_oset_contains(oset, rz_sv_from_cstr(cstrkey))
#define rz_oset_get_byidx(oset, idx)         (RZ_ASSERT((idx <= (oset)->count) && (0 <= idx)), (oset)->items[idx].key)

#endif  // defined __RZ_SET_H__

#ifdef RZ_SET_IMPLEMENTATION

rz_set_hasher rz_hasher_func = rz_default_hasher;
rz_hash_t     rz_default_hasher(const void *key, size_t sz) {
    // FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/)
    rz_hash_t h = 14695981039346656037ULL;  // FNV_OFFSET 64 bit
    for (size_t i = 0; i < sz; ++i) {
        h = h ^ ((unsigned char *)key)[i];
        h = h * 1099511628211ULL;  // FNV_PRIME 64 bit
    }
    return h;
}
static inline bool __rz_contains_internal(rz_oset_t *oset, rz_hash_t hash, int sz) {
    if (rz_oset_is_empty(oset)) return false;
    for (size_t i = 0; i < oset->count; i++) {
        if (sz == oset->items[i].size && hash == oset->items[i].hash) return true;
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
rz_status_t rz_oset_insert(rz_oset_t *oset, const void *key, int sz) {
    rz_hash_t hash = rz_hasher_func(key, sz);
    if (__rz_contains_internal(oset, hash, sz)) return RZ_ERR;
    if (oset->count >= oset->capacity) {
        size_t last_cap = oset->capacity;
        oset->capacity  = oset->capacity == 0 ? RZ_DA_INIT_CAP : oset->capacity * 2;
        oset->items     = (rz_oset_item_t *)RZ_REALLOC(oset->items, oset->capacity * sizeof(*oset->items));
        RZ_ASSERT_ALLOC(oset->items);
    }
    oset->items[oset->count++] = (rz_oset_item_t){.size = sz, .hash = hash, .key = key};
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
