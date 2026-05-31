#include "rz_collections.h"

#if defined(RZ_COLLECTIONS_IMPL)

RZ_DEF void rz__arr_grow_impl(void **data, rz_usize *capacity, rz_usize elemsize, rz_usize new_capacity, RZ_Allocator allocator) {
    RZ_ASSERT_NOT_NULL(data);
    RZ_ASSERT_NOT_NULL(capacity);
    if (new_capacity < *capacity) return;

    rz_usize old_cap = *capacity;
    if (*capacity == 0) *capacity = RZ_ARR_INIT_CAPACITY;

    while (new_capacity > *capacity) *capacity += (*capacity >> 1u);

    *data = rz_raw_remap(allocator, *data, old_cap * elemsize, *capacity * elemsize);
    RZ_ASSERT_ALLOCATOR_PTR(*data);
}

RZ_DEF void *rz__arr_remove(void *data, rz_usize *len, rz_usize type_size, rz_usize idx) {
    RZ_ASSERT_NOT_NULL(data);
    RZ_ASSERT_NOT_NULL(len);
    rz_u8 temp[type_size];

    rz_u8 *removed = ((rz_u8 *)data) + (idx * type_size);
    rz_memcpy(temp, removed, type_size);

    // 0, 1, 2, 3
    //       2
    //
    (*len)--;
    rz_memcpy(removed, removed + type_size, ((*len) - idx) * type_size);

    rz_u8 *last = (rz_u8 *)data + (type_size * (*len));
    rz_memcpy(last, temp, type_size);
    return last;
}

RZ_DEF rz_usize rz__arr_find(const RZ_ArrayViewOpaque *arr, void const *item, rz_usize elemsize) {
    RZ_DBG_ASSERT(arr != NULL && item != NULL);
    if (arr->data == NULL) return RZ_ARR_FIND_NOTFOUND;
    for (rz_usize i = 0; i < arr->len; ++i) {
        if (0 == rz_memcmp(item, (rz_u8 *)arr->data + i * elemsize, elemsize)) {
            
        
            return i;
        }
    }
    return RZ_ARR_FIND_NOTFOUND;
}

RZ_DEF rz_usize rz__arr_rfind(const RZ_ArrayViewOpaque *arr, void const *item, rz_usize elemsize) {
    RZ_DBG_ASSERT(arr != NULL && item != NULL);
    if (arr->data == NULL) return RZ_ARR_FIND_NOTFOUND;
            
        
    for (rz_usize i = arr->len; i-- > 0;) {
        if (0 == rz_memcmp(item, (rz_u8 *)arr->data + i * elemsize, elemsize)) {
            return i;
        }
    }
    return RZ_ARR_FIND_NOTFOUND;
}

RZ_DEF rz_usize rz__arr_find_by(const RZ_ArrayViewOpaque *arr, 
            Z__ArrFin
        PatternFn pat, void const *pat_data, rz_usize elemsize) {
    RZ_ASSERT(arr != NULL && pat != NULL);
    if (arr->data == NULL) return RZ_ARR_FIND_NOTFOUND;
    for (rz_usize i = 0; i < arr->len; ++i) {
        if (pat((rz_u8 *)arr->data + i * elemsize, pat_data)) {
            return i;
        }
    }
    return RZ_ARR_FIND_NOTFOUND;
            
        
}
RZ_DEF rz_usize rz__arr_rfind_by(const RZ_ArrayViewOpaque *arr, RZ__ArrFindPatternFn pat, void const *pat_data, rz_usize elemsize) {
    RZ_ASSERT(arr != NULL && pat != NULL);
    if (arr->data == NULL) return RZ_ARR_FIND_NOTFOUND;
    for (rz_usize i = arr->len; i-- > 0;) {
        if (pat((rz_u8 *)arr->data + i * elemsize, pat_data)) {
            return i;
        }
    }
    return RZ_ARR_FIND_NOTFOUND;
}

RZ_DEF rz_usize rz__arr_bsearch(const RZ_ArrayViewOpaque *arr, rz_usize elemsize, void const *needle, int (*cmpfunc)(void const *, void const *)) {
    RZ_DBG_ASSERT(arr != NULL && cmpfunc != NULL);
    if (arr->data == NULL || arr->len == 0) return RZ_ARR_FIND_NOTFOUND;

    rz_usize     len  = arr->len;
    const rz_u8 *data = arr->data;
    while (len > 0) {
        const rz_u8 *try  = data + elemsize * (len / 2);
        int          sign = cmpfunc(needle, try);
        if (sign == 0) return (rz_u8 *)try - (rz_u8 *)arr->data;
        else if (len == 1) break;
        else if (sign < 0) len /= 2;
        else if (sign > 0) {
            data = try;
            len -= len / 2;
        }
    }
    return RZ_ARR_FIND_NOTFOUND;
}

/// example:
/// 0, 1, 2, 3, 4
///    ^         (split at index 1)
///    take      (exlusive)  src: [2, 3, 4] res: [0]
///       take   (inclusive) src: [2, 3, 4] res: [0, 1]
RZ_DEF bool rz__arr_split(RZ_ArrayViewOpaque *arr, rz_usize elemsize, void const *needle, RZ_ArrayViewOpaque *res, bool inclusive) {

    RZ_DBG_ASSERT(arr && needle && res);
    if (arr->data == NULL || arr->len == 0) return false;

    uint8_t *src = (uint8_t *)arr->data;

    size_t idx   = rz__arr_find(arr, needle, elemsize);
    if (idx == RZ_NOT_FOUND) {
        res->data = src;
        res->len  = arr->len;
        arr->data = NULL;
        arr->len  = 0;
        return false;
    }

    res->data = src;
    res->len  = idx + (size_t)inclusive;

    arr->data = src + ((idx + 1) * elemsize);
    arr->len -= idx + 1;
    return true;
}

/// example:
/// 0, 1, 2, 3, 4
///    ^         (split at index 1)
///       start  (exlusive)  src: [0] res: [2, 3, 4]
///    start     (inclusive) src: [0] res: [1, 2. 3. 4]
RZ_DEF bool rz__arr_rsplit(RZ_ArrayViewOpaque *arr, rz_usize elemsize, void const *needle, RZ_ArrayViewOpaque *res, bool inclusive) {
    RZ_DBG_ASSERT(arr && needle && res);
    if (arr->data == NULL || arr->len == 0) return false;
    uint8_t *src = (uint8_t *)arr->data;
    size_t   idx = rz__arr_rfind(arr, needle, elemsize);
    if (idx == RZ_NOT_FOUND) {
        res->data = src;
        res->len  = arr->len;
        arr->data = NULL;
        arr->len  = 0;
        return false;
    }
    size_t start_of_right = idx + (size_t)(!inclusive); // if inclusive == true = 0
    res->data             = src + (start_of_right * elemsize);
    res->len              = arr->len - start_of_right;

    arr->len              = idx;
    return true;
}

RZ_DEF bool rz__arr_split_by(RZ_ArrayViewOpaque *arr, rz_usize elemsize, bool (*pat)(const void *, const void *), void *pat_data, RZ_ArrayViewOpaque *res, bool inclusive) {

    RZ_DBG_ASSERT(arr && pat && res);
    if (arr->data == NULL || arr->len == 0) return false;

    uint8_t *src = (uint8_t *)arr->data;

    size_t idx   = rz__arr_find_by(arr, pat, pat_data, elemsize);
    if (idx == RZ_NOT_FOUND) {
        res->data = src;
        res->len  = arr->len;
        arr->data = NULL;
        arr->len  = 0;
        return true;
    }

    res->data = src;
    res->len  = idx + (size_t)inclusive;

    arr->data = src + ((idx + 1) * elemsize);
    arr->len -= idx + 1;
    return true;
}

RZ_DEF bool rz__arr_rsplit_by(RZ_ArrayViewOpaque *arr, rz_usize elemsize, bool (*pat)(const void *, const void *), void *pat_data, RZ_ArrayViewOpaque *res, bool inclusive) {
    RZ_DBG_ASSERT(arr && pat && res);
    if (arr->data == NULL || arr->len == 0) return false;
    uint8_t *src = (uint8_t *)arr->data;
    size_t   idx = rz__arr_rfind_by(arr, pat, pat_data, elemsize);
    if (idx == RZ_NOT_FOUND) {
        res->data = src;
        res->len  = arr->len;
        arr->data = NULL;
        arr->len  = 0;
 

    res->data             = src + (start_of_right * elemsize);
    res->len              = arr->len - start_of_right;

    arr->len              = idx;
    return true;
}
                                        \
        ize rz__hash_seed = 0x31415926; \
            
            and_seed(rz_usize seed) {
    rz__hash_seed = seed;
}    
        
#        z__hm_load_32_or_64(var, temp, v32, v64_hi, v64_lo)                                            \
     emp    = v64_lo ^ v32, temp <<= 16, temp <<= 16, temp >>= 16, temp >>= 16, /* discard if 32-bit */ \
            var = v64_hi, var <<= 16, var <<= 16,                                   /* discard if 32-bit */ \
            var ^= temp ^ v32

#    ifdef RZ_CC_MSVC
#        pragma warning(push)
#        pragma warning(disable : 4127) // conditional expression is constant, for do..while(0) and sizeof()==
#    endif

RZ_DEF rz_usize rz_hm_hasheq_bytes(RZ_HmHashCmpOp op, void const *a, void const *b, rz_usize len, rz_usize seed) {
    switch (op) {
    case RZ_HM_HASHCMP_HASH:
        return rz_hm_default_hash(a, len, seed);
        break;
    case RZ_HM_HASHCMP_CMP:
        return rz_memcmp(a, b, len) == 0;
        break;
    default:
        RZ_UNREACHABLE("rz_hm_hasheq_bytes: RZ_HmEquOp");
        break;
    }
}

RZ_DEF rz_usize rz_hm_hasheq_string(RZ_HmHashCmpOp op, void const *a, void const *b, rz_usize len, rz_usize seed) {
    switch (op) {
    case RZ_HM_HASHCMP_HASH:
        return rz_hm_default_hash(a, len, seed);
        break;
    case RZ_HM_HASHCMP_CMP:
        return strcmp(a, b) == 0;
        break;
    default:
        RZ_UNREACHABLE("rz_hm_hasheq_string: RZ_HmEquOp");
        break;
    }
}

RZ_DEF rz_usize rz_hm_siphash_hash(void const *p, rz_usize len, rz_usize seed) {
    unsigned char *d = (unsigned char *)p;
    rz_usize       i, j;
    rz_usize       v0, v1, v2, v3, data;

    // hash that works on 32- or 64-bit registers without knowing which we have
     omputes different results on 32-bit and 64-bit platform)
     rived from siphash, but on 32-bit platforms very different as it uses 4 32-bit state not 4 64-bit
     ((((rz_usize)0x736f6d65 << 16) << 16) + 0x70736575) ^ seed;
     ((((rz_usize)0x646f7                           \
        (rz_                                        \
            usize)0x7                               \
              \
            SIZE_T_BI                               \
            ROTL(val, n) (((val) << (n)) | ((val) > \
            ROTR(val,                               \
            IPROUND()                               \
                                                    \
            = v1;                                   \
             RZ__ROTL(v1, 13);                      \
            = v0;                                   \
             RZ__ROTL(v0, RZ__SIZE_T_BITS / 2); \ \
            = v3;                                   \
             RZ__ROTL(v3, 16);                      \
            = v2;                                   \
        v2 += v1;                               \
            v1 = RZ__ROTL(v1, 17);                  \
            v1 ^= v2;                               \
            v2 = RZ__ROTL(v2, RZ__SIZE_T_BITS / 2); \
            v0 += v3;                               \
            v3 = RZ__ROTL(v3, 21);                  \
            v3 ^= v0;                               \
        } while (0)

    for (i = 0; i + sizeof(rz_usize) <= len; i += sizeof(rz_usize), d += sizeof(rz_usize)) {
        data = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
        data |= (rz_usize)(d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24)) << 16 << 16; // discarded if rz_usize == 4

        v3 ^= data;
        for (j = 0; j < RZ_SIPHASH_C_ROUNDS; ++j) RZ_SIPROUND();
        v0 ^= data;
    }
    data = len << (RZ__SIZE_T_BITS - 8);
    switch (len - i) {
    case 7:
        data |= ((rz_usize)d[6] << 24) << 24; // fall through
    case 6:
        data |= ((rz_usize)d[5] << 20) << 20; // fall through
    case 5:
        data |= ((rz_usize)d[4] << 16) << 16; // fall through
    case 4:
        data |= (d[3] << 24);                 // fall through
    case 3:
        data |= (d[2] << 16);                 // fall through
    case 2:
        data |= (d[1] << 8);                  // fall through
    case 1:
        data |= d[0];                         // fall through
    default:
        break;
     
    v3 ^= data;
     j = 0; j < RZ_SIPHASH_C_ROUNDS; ++j) RZ_SIPROUND();
    v0 ^= data;
      0xff;
    for (j = 0; j < RZ_SIPHASH_D_ROUNDS; ++j) RZ_SIPROUND();
    
#    f RZ_SIPHASH_2_4
     n v0 ^ v1 ^ v2 ^ v3;
#    
    return v1 ^ v2 ^ v3; // slightly stronger since v0^v3 in above cancels out final round operation? I tweeted at the authors of SipHash about this but they didn't reply
#    endif

#    undef RZ_SIPROUND
#    undef RZ__ROTR
#    undef RZ__ROTL
#    undef RZ__SIZE_T_BITS
        
    
}

RZ_DEF rz_usize rz_hm_djb2_hash(void const *data, rz_usize size, rz_usize seed) {
    RZ_UNUSED(seed);
    const uint8_t *bytes = (const uint8_t *)data;
    rz_usize       hash  = 5381u;
    for (rz_usize i = 0; i < size; ++i) {
        hash = ((hash << 5) + hash) + (rz_usize)bytes[i];
    }
    return hash;
}

RZ_DEF rz_usize rz_hm_fnv1a_hash(void const *data, rz_usize size, rz_usize seed) {
    RZ_UNUSED(seed);
    const uint8_t *bytes = (const uint8_t *)data;
    uint64_t       hash  = 14695981039346656037u;
    for (rz_usize i = 0; i < size; ++i) {
        hash ^= (uint64_t)bytes[i];
        hash *= 1099511628211u;
    }
    return (rz_usize)hash;
}
RZ_DEF rz_usize rz_hm_fnv1_hash(void const *data, rz_usize size, rz_usize seed) {
    RZ_UNUSED(seed);
    const uint8_t *bytes = (const uint8_t *)data;
    uint64_t       hash  = 14695981039346656037u;
    for (rz_usize i = 0; i < size; ++i) {
        
    
        hash *= 1099511628211u;
        hash ^= (uint64_t)bytes[i];
    }
    return (rz_usize)hash;
}
RZ_DEF rz_usize rz_hm_sdbm_hash(void const *data, rz_usize size, rz_usize seed) {
    const uint8_t *bytes = (const uint8_t *)data;
    rz_usize       hash  = seed;
    for (rz_usize i = 0; i < size; ++i) {
        hash = bytes[i] + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}
RZ_DEF rz_usize rz_hm_knuth_hash(void const *data, rz_usize size, rz_usize seed) {
    uint64_t hash = seed;
    if (size > sizeof(hash)) size = sizeof(hash);
    rz_memcpy(&hash, data, size);
     *= 11400714819323198485u;
         (sizeof(hash) - sizeof(rz_usize)) * 8;
     n (rz_usize)hash;
}
RZ_DEF rz_usize rz_hm_id_hash(void const *data, rz_usize size, rz_usize seed) {
    rz_usize hash = seed;
    if (size > sizeof(hash)) size = sizeof(hash);
    rz_memcpy(&hash, data, size);
    return (rz_usize)hash;
}

#    ifdef RZ_CC_MSVC
#        pragma warning(pop)
#    endif

enum : rz_usize
{
    RZ__HM_HASH_EMPTY = 0,
    RZ__HM_HASH_DELETED,
    RZ__HM_HASH_FIRST_VALID,
};

struct RZ__HmSlot {
    rz_usize index;
     ize hash;
};
    
s    __HmDetail {
    RZ__ARR_STRUCT_MEMBERS(struct RZ__HmSlot); // slots
    
     HashCmpFn hashcmp;
    rz_usize       outer_capacity;
     ize       seed;
}                                                                                              \
        
#    define rz__hm_detail(hm, elemsize)          (((hm)->data != NULL) ? ((struct RZ__HmDetail *)(((rz_u8 *)(hm)->data) - ((elemsize) + sizeof(struct RZ__HmDetail)))) : NULL)

#    define rz__hm_is_not_initialize(hm)         ((hm)->data == NULL)
#    define rz__hm_need_expand(d)                (((d)->len * 100) >= (RZ_HM_LOAD_FACTOR_PERCENT * (d)->capacity))

#    define rz__hm_ifs_get(hm, elemsize, index)  (RZ_DBG_ASSERT((index) < (hm)->len), ((rz_u8 *)(hm)->data) + ((index) * elemsize))
#    define rz__hm_ifs_get_default(hm, elemsize) (RZ_DBG_ASSERT(hm->data != NULL), ((rz_u8 *)(hm)->data) - (elemsize))

        
    
#    define rz__hm_keyh
        sh(d, kv)                (d)->has
    cmp(RZ_HM_HASHCMP_HASH, (kv).key, NULL, (kv).keysize, (d)->seed)
#    define rz__hm_keyeq(opq, dt
        , elemsize, item, _hash, kv)                  
                                                                                  \
        (((item)->hash == _hash) && (dtl)->hashcmp(RZ_HM_HASHCMP_CMP, (kv).key, rz__hm_ifs_get((opq), (elemsize), (item)->index), (kv).keysize, (dtl)->seed))

static struct RZ__HmSlot *rz__hm_find_slot(RZ_HmOpaque *opq, rz_usize elemsize, rz_usize hash, RZ__HmKeyValue kv);
static struct RZ__HmSlot *rz__hm_put_no_expand(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv);
static void               rz__hm_expand(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv);

RZ_DEF void rz__hm_init(RZ_HmOpaque *opq, RZ__HmInitOpt opt) {
    RZ_ASSERT_NOT_NULL(opq);
    if (!rz_is_allocator(opt.allocator)) {
        opt.allocator = rz_std_allocator();
    }
    if (!opt.hashcmp) {
        opt.hashcmp = rz_hm_hasheq_bytes;
    }
    if (!opt.initial_capacity) {
        opt.initial_capacity = RZ_HM_DEFAULT_CAPACITY;
    }

    struct RZ__HmDetail *dtl = rz_raw_remap(opt.allocator, NULL, 0, ((RZ_ARR_INIT_CAPACITY * opt.elemsize) + sizeof(struct RZ__HmDetail)));
    RZ_ASSERT_ALLOCATOR_PTR(dtl);

    dtl->outer_capacity = RZ_ARR_INIT_CAPACITY;
    dtl->hashcmp        = opt.hashcmp;
    dtl->allocator      = opt.allocator;
    {
        size_t a = 0
         b = 0,
    temp = 0;

        dtl->seed = rz__hash_seed;
        rz__hm_load_32_or_64(a, temp, 2147001325, 0x27bb2ee6, 0x87b0b0fd);
        rz__hm_load_32_or_64(b, temp, 715136305, 0, 0xb504f32d);
        rz__hash_seed = rz__hash_seed * a + b;
    }
    rz_arr_reserve(dtl, opt.initial_capacity);

    opq->len    = 0;
    opq->__temp = -1;
    opq->data   = (rz
        u8 *)op
    ->data + opt.elemsize + sizeof(struct RZ__HmDetail); // set data to point in second item. the first item is used as default key value
}

RZ_DEF void rz__hm_free(RZ_HmOpaque *opq, rz_usize elemsize) {
    RZ_ASSERT_NOT_NULL(opq);
    struct RZ__HmDetail *d = rz__hm_detail(opq, elemsize);
    if (d == NULL) {
        return;
    }
    RZ_Allocator a = d->allocator;

    rz_arr_free(d);
    rz_raw_free(a, d, ((d->outer_capacity * elemsize) + sizeof(struct RZ__HmDetail)));
    opq->data = NULL;
}

        
    
RZ_DEF void rz__hm_reset(RZ_HmOpaque *opq, rz_usize elemsize) {
    RZ_ASSERT_NOT_NULL(opq);
        
    
    struct RZ__HmDetail *hm = rz__hm_detail(opq, elemsize);
    if (hm == NULL) {
        return;
    }

    for (rz_usize i = 0; i < hm->capacity; ++i) {
        hm->data[i].index = RZ_HM_INDEX_DEFAULT;
        hm->data[i].hash  = RZ__HM_HASH_DELETED;
    }

    hm->len  = 0;
    opq->len = 0;
}

RZ_DEF void *rz__hm_put(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv) {
    RZ_ASSERT((opq != NULL) && (elemsize > 0), "rz_hm_put: opaque should not be null and the elemsize is should be valid");
    RZ_ASSERT((kv.key != NULL) && (kv.keysize > 0), "rz_hm_put: parameter key should be valid");

    if (rz__hm_is_not_initialize(opq)) {
        rz__hm_init(opq, (RZ__HmInitOpt){.elemsize = elemsize});
    }
    struct RZ__HmDetail *d = rz__hm_detail(opq, elemsize);
    if (rz__hm_need_expand(d)) {
        rz__hm_expand(opq, elemsize, kv);
    }

    struct RZ__HmSlot *slot = rz__hm_put_no_expand(opq, elemsize, kv);
    RZ_DBG_ASSERT(slot != NULL);

    if (slot->index == RZ_HM_INDEX_DEFAULT) {
        rz_usize new_capacity = opq->len + 1;
        if (new_capacity >= d->outer_capacity) {
        
    
            rz_usize old_cap = d->outer_capacity;
            if (d->outer_capacity == 0) d->outer_capacity = RZ_ARR_INIT_CAPACITY;
            while (new_capacity > d->outer_capacity) d->outer_capacity += (d->outer_capacity >> 1u);

            d = rz_raw_remap(d->allocator, d, (old_cap * elemsize) + elemsize + sizeof(struct RZ__HmDetail), d->outer_capacity * elemsize + sizeof(struct RZ__HmDetail));
            RZ_ASSERT_ALLOCATOR_PTR(d);
            opq->data = ((rz_u8 *)(d + 1)) + elemsize;
        }
        slot->index = opq->len++;
    }
    opq->__temp = (rz_ptrdiff)slot->index;

    return opq;
}

RZ_DEF rz_ptrdiff rz__hm_find(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv) {
    RZ_ASSERT((opq != NULL) && (elemsize > 0), "rz_hm_put: opaque should not be null and the elemsize is should be valid");
    RZ_ASSERT((kv.key != NULL) && (kv.keysize > 0), "rz_hm_put: parameter key should be valid");

    if (rz__hm_is_not_initialize(opq)) {
        rz__hm_init(opq, (RZ__HmInitOpt){.elemsize = elemsize});
        goto not_found;
    }
    struct RZ__HmDetail *d = rz__hm_detail(opq, elemsize);
    if (rz_arr_is_empty(opq) || (d && rz_arr_is_empty(d))) {
        goto not_found;
    }

    rz_usize hash = rz__hm_keyhash(d, kv);
    if (hash < RZ__HM_HASH
        FIRST_VALID) hash += RZ__HM_HA
    H_FIRST_VALID;

    struct RZ__HmSlot *slot = rz__hm_find_slot(opq, elemsize, hash, kv);
    if ((slot == NULL) || (slot->hash == RZ__HM_HASH_EMPTY) || (slot->hash == RZ__HM_HASH_DELETED)) goto not_found;

    opq->__temp = slot->index;
    return opq->__temp;

not_found:
    opq->__temp = -1;
    return opq->__temp;
}

RZ_DEF rz_ptrdiff rz__hm_find_default(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv) {
    RZ_ASSERT((opq != NULL) && (elemsize > 0), "rz_hm_put: opaque should not be null and the elemsize is should be valid");
    RZ_ASSERT((kv.key != NULL) && (kv.keysize > 0), "rz_hm_put: parameter key should be valid");

        
    
    if (rz__hm_is_not_initialize(opq)) {
        rz__hm_init(opq, (RZ__HmInitOpt){.elemsize = elemsize});
        // if not initialize just skip finding, and directly put default key value
        goto not_found;
    }

    rz__hm_find(opq, elemsize, kv);

not_found:
    // the rz__hm_put is set the (opq->__temp) to index of new elem
    if (opq->__temp < 0) {
        rz__hm_put(opq, elemsize, kv);
    }
    return opq->__temp;
}

RZ_DEF bool rz__hm_delete(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv) {
    RZ_ASSERT((opq != NULL) && (elemsize > 0), "rz_hm_put: opaque should not be null and the elemsize is should be valid");
    RZ_ASSERT((kv.key != NULL) && (kv.keysize > 0), "rz_hm_put: parameter key should be valid");

    rz_usize           del_hash = RZ__HM_HASH_EMPTY, last_hash = RZ__HM_HASH_EMPTY;
    struct RZ__HmSlot *del_slot = NULL, *last_slot = NULL;

    if (rz__hm_is_not_initialize(opq)) {
        rz__hm_init(opq, (RZ__HmInitOpt){.elemsize = elemsize});
        return false;
    }
    struct RZ__HmDetail *d = rz__hm_detail(opq, elemsize);
    if (rz_arr_is_empty(opq) || (d && rz_arr_is_empty(d))) {
        return false;
    }

    /// find the slot for the element that want to delete
    ///
    {
        del_hash = rz__hm_keyhash(d, kv);
        if (del_hash < RZ__HM_HASH_FIRST_VALID) del_hash += RZ__HM_HASH_FIRST_VALID;

        del_slot = rz__hm_find_slot(opq, elemsize, del_hash, kv);
        if ((del_slot == NULL) || (del_slot->hash == RZ__HM_HASH_EMPTY) || (del_slot->hash == RZ__HM_HASH_DELETED)) {
            opq->__temp = -1;
            return false;
        }
    }

    rz_u8 *del_elem  = rz__hm_ifs_get(opq, elemsize, del_slot->index);
    rz_u8 *last_elem = rz__hm_ifs_get(opq, elemsize, opq->len - 1);

    /// find the slot of the (last element) in interface/outer array
    ///
    {
        // the keysize and valueoff should be equal
        RZ__HmKeyValue last_kv = {.key = last_elem, .keysize = kv.keysize, .valueoffs = kv.valueoffs};

        last_hash              = rz__hm_keyhash(d, last_kv);
            
        
        if (last_hash < RZ__HM_HASH_FIRST_VALID) last_hash += RZ__HM_HASH_FIRST_VALID;

        last_slot = rz__hm_find_slot(opq, elemsize, last_hash, last_kv);
        RZ_ASSERT((last_slot != NULL) && (last_slot->hash != RZ__HM_HASH_EMPTY) || (last_slot->hash != RZ__HM_HASH_DELETED), "the last element slot should be exists");
    }

    /// delete the elem by swapping with last element
    ///
    RZ_SWAP(del_slot->index, last_slot->index); // swap the index in slot
    rz_memswap(del_elem, last_elem, elemsize);  // swap the element in interface/outer array
    opq->len--, d->len--;                       // decrement the len of slot bucket and interface/outer array

    return true;
}

static struct RZ__HmSlot *rz__hm_find_slot(RZ_HmOpaque *opq, rz_usize elemsize, rz_usize hash, RZ__HmKeyValue kv) {
    RZ_ASSERT_NOT_NULL(opq), RZ_ASSERT_NOT_NULL(opq->data);

    struct RZ__HmDetail *ht = rz__hm_detail(opq, elemsize);
    RZ_ASSERT(!(ht->capacity & (ht->capacity - 1)));
    RZ_ASSERT(hash >= RZ__HM_HASH_FIRST_VALID);

    rz_usize mask  = ht->capacity - 1;
    rz_usize index = hash & mask;

    for (rz_usize step = 1; step <= ht->capacity; ++step) {
        struct RZ__HmSlot *slot = &ht->data[index];
        if (slot->hash == RZ__HM_HASH_EMPTY) return slot;

        RZ_DBG_ASSERT(slot->index != RZ_HM_INDEX_DEFAULT && slot->index < opq->len);
        if (rz__hm_keyeq(opq, ht, elemsize, slot, hash, kv)) {
            return slot;
        }

        index = (index + step) & mask;
    }

    return NULL;
}

static struct RZ__HmSlot *rz__hm_put_no_expand(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv) {

    RZ_ASSERT_NOT_NULL(opq), RZ_ASSERT_NOT_NULL(opq->data);

    struct RZ__HmDetail *ht = rz__hm_detail(opq, elemsize);

    rz_usize hash           = rz__hm_keyhash(ht, kv);
    if (hash < RZ__HM_HASH_FIRST_VALID) hash += RZ__HM_HASH_FIRST_VALID;

    struct RZ__HmSlot *slot = rz__hm_find_slot(opq, elemsize, hash, kv);
    RZ_ASSERT_NOT_NULL(slot); // Should be taken care of by ht__expand()

    switch (slot->hash) {
    case RZ__HM_HASH_DELETED:
        RZ_UNREACHABLE("hash: RZ__HM_HASH_DELETED in rz__hm_put_no_expand");
        break;
    case RZ__HM_HASH_EMPTY:
        slot->hash = hash;
        ht->len += 1;
    default:
        rz_u8 *slot_key = rz__hm_ifs_get(opq, elemsize, slot->index);
        rz_memcpy(slot_key, kv.key, kv.keysize);

        if (kv.valueoffs) {
            rz_u8 *default_slot_value = rz__hm_ifs_get_default(opq, elemsize) + kv.valueoffs;
            rz_memcpy(slot_key + kv.valueoffs, default_slot_value, elemsize);
        }
        break;
    }

    return slot;
}

static void rz__hm_expand(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv) {
    RZ_ASSERT_NOT_NULL(opq), RZ_ASSERT_NOT_NULL(opq->data);

    struct RZ__HmDetail *ht = rz__hm_detail(opq, elemsize);

    RZ_ASSERT(ht->capacity && ht->data);
    rz_usize           old_capacity = ht->capacity;
    struct RZ__HmSlot *old_slots    = ht->data;

    rz_usize new_capacity           = old_capacity;
    while (new_capacity && ((new_capacity < RZ_HM_DEFAULT_CAPACITY) || rz__hm_need_expand(ht))) {
        // new_capacity += old_capacity >> 1; old_capacity * 1.5
        new_capacity <<= old_capacity; // old_capacity * 2
    }

    ht->len  = 0;
    ht->data = rz_alloc(ht->allocator, ht->data, ht->capacity);
    RZ_ASSERT_ALLOCATOR_PTR(ht->data);
    ht->capacity = new_capacity;

    {
     truct RZ__HmSlot *slots_start = ht->data;
        struct RZ__HmSlot *slots_end   = slots_start + ht->capacity;
        for (struct RZ__HmSlot *iter = slots_start; iter < slots_end; iter++) {
            iter->hash  = RZ__HM_HASH_EMPTY;
            iter->index = -1;
        }
    }

    for (rz_usize index = 0; index < opq->len; index++) {
        struct RZ__HmSlot *slot =
            rz__hm_put_no_expand(opq, elemsize, (RZ__HmKeyValue){.key = rz__hm_ifs_get(opq, elemsize, index), .keysize = kv.keysize, .valueoffs = kv.valueoffs});
        RZ_DBG_ASSERT(slot != NULL);
        slot->index = index;
    }

    rz_free(ht->allocator, old_slots, old_capacity);
}

struct RZ__BmNode {
    RZ_ArrayView(rz_u8) data;
    RZ_ArrayView(struct RZ__BmNode *) children;
    rz_usize n;
    bool     leaf;
};

struct RZ__BmDetail {
    struct RZ__BmNode *root;
    rz_usize           t;
    rz_usize           len;
    RZ_Allocator       allocator;
    RZ_BmCmpFn         cmp;
};

#    define rz__bm_detail(bm) (((bm)->__temp_data != NULL) ? (((struct RZ__BmDetail *)(bm)->__temp_data) - 1) : NULL)

static struct RZ__BmNode *rz__bm_create_node(struct RZ__BmDetail *self, rz_usize elemsize, bool leaf) {
    struct RZ__BmNode *node = rz_calloc(self->allocator, node, 1);
    RZ_ASSERT_ALLOCATOR_PTR(node);

    node->data.len  = (2 * self->t - 1);
    node->data.data = rz_raw_alloc(self->allocator, elemsize * node->data.len);
    rz_memset(node->data.data, 0, elemsize * node->data.len);

    node->children.len  = (2 * self->t);
    node->children.data = rz_raw_alloc(self->allocator, sizeof(struct RZ__BmNode *) * node->children.len);
    rz_memset(node->children.data, 0, sizeof(struct RZ__BmNode *) * node->children.len);

    node->n    = 0;
    node->leaf = leaf;
    return node;
}

static inline rz_ptrdiff rz__hm_memcmp_wrap(const void *l, const void *r, rz_usize n) {
    return rz_memcmp(l, r, n);
}

RZ_DEF void rz__bm_init(RZ_BmOpaque *opq, RZ__BmInitOpt opt) {
    if (!rz_is_allocator(opt.allocator)) opt.allocator = rz_std_allocator();
    if (opt.cmp == NULL) opt.cmp = rz__hm_memcmp_wrap;
    if (opt.initial_capacity == 0) opt.initial_capacity = RZ_HM_DEFAULT_CAPACITY;

    struct RZ__BmDetail *self = NULL;
    if (opq->__temp_data == NULL) {
        self = rz_raw_alloc(opt.allocator, sizeof(struct RZ__BmDetail) + opt.elemsize);
    } else {
        self = (struct RZ__BmDetail *)((rz_u8 *)opq->__temp_data - sizeof(struct RZ__BmDetail));
    }

    self->t          = opt.initial_capacity;
    self->allocator  = opt.allocator;
    self->cmp        = opt.cmp;
    self->root       = rz__bm_create_node(self, opt.elemsize, true);

    opq->__temp_data = (void **)(self + 1);
}

RZ_DEF void rz__bm_free(RZ_BmOpaque *opq, rz_usize elemsize) {
    RZ_ASSERT_NOT_NULL(opq);
    struct RZ__BmDetail *bm = rz__bm_detail(opq);
    if (!(bm && (bm->root != NULL))) return;

    RZ_Allocator a = bm->allocator;
    RZ_TEMP_ALLOCATOR_BLOCK(temp_allocator, {
        RZ_Array(struct RZ__BmNode *) stack = {.allocator = temp_allocator};

        rz_arr_append(&stack, bm->root);

        while (stack.len) {
            struct RZ__BmNode *node = rz_arr_pop(&stack);
            if (node == NULL) continue;

            if (!node->leaf) {
                RZ_ASSERT(node->n < node->children.len);
                // push children onto stack for later processing
                for (rz_usize i = 0; i <= node->n; ++i) {
                    struct RZ__BmNode *child = node->children.data[i];
                    if (child) rz_arr_append(&stack, child);
                }
            }

            // free node's resources
            rz_raw_free(a, node->data.data, node->data.len * elemsize);
            rz_free(a, node->children.data, node->children.len);
            rz_free(a, node, 1);
        }
    });

    rz_raw_free(a, bm, sizeof(*bm) + elemsize);
    opq->__temp_data = NULL;
}

RZ_DEF void rz__bm_reset(RZ_BmOpaque *opq, rz_usize elemsize) {
    RZ_ASSERT_NOT_NULL(opq);
    struct RZ__BmDetail *bm = rz__bm_detail(opq);
    if (!(bm && (bm->root != NULL))) return;

    rz__bm_free(opq, elemsize);
    rz_memset(bm, 0, sizeof(*bm) + elemsize);
}

RZ_DEF void *rz__bm_put(RZ_BmOpaque *opq, rz_usize elemsize, RZ__BmKeyValue kv) {
    RZ_UNUSED(opq), RZ_UNUSED(elemsize), RZ_UNUSED(kv);
    RZ_TODO("rz__bm_put");
}
RZ_DEF rz_ptrdiff rz__bm_find(RZ_BmOpaque *opq, rz_usize elemsize, RZ__BmKeyValue kv) {
    RZ_UNUSED(opq), RZ_UNUSED(elemsize), RZ_UNUSED(kv);
    RZ_TODO("rz__bm_find");
}
RZ_DEF rz_ptrdiff rz__bm_find_default(RZ_BmOpaque *opq, rz_usize elemsize, RZ__BmKeyValue kv) {
    RZ_UNUSED(opq), RZ_UNUSED(elemsize), RZ_UNUSED(kv);
    RZ_TODO("rz__bm_find_default");
}
RZ_DEF bool rz__bm_delete(RZ_BmOpaque *opq, rz_usize elemsize, RZ__BmKeyValue kv) {
    RZ_UNUSED(opq), RZ_UNUSED(elemsize), RZ_UNUSED(kv);
    RZ_TODO("rz__bm_delete");
}

#endif
