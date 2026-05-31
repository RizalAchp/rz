#pragma once
#ifndef RZ_HASHMAP_H
#    define RZ_HASHMAP_H
#    include "rz_allocator.h"
#    include "rz_common.h"

// clang-format off
///////////////
/// Array and Vec Macors helpers
///
#    ifndef RZ_ARR_INIT_CAPACITY
#        define RZ_ARR_INIT_CAPACITY 4U
#    endif

#    define RZ__ARRVIEW_STRUCT_MEMBERS(T)                  \
        /* data - the elements or items for the array */ \
        T       *data;                                   \
        /* len - the size or length of the `data` */     \
        rz_usize len

#    define RZ__ARR_STRUCT_MEMBERS(T)                         \
        RZ__ARRVIEW_STRUCT_MEMBERS(T);                          \
        /* capacity - the capacity of the `data` allocated */ \
        rz_usize     capacity;                                \
        /* allocator - the allocator of the dynamic array*/   \
        RZ_Allocator allocator

#    define RZ_ArrayView(T)                  \
        struct {                         \
            RZ__ARRVIEW_STRUCT_MEMBERS(T); \
        }
#    define RZ_Array(T)                  \
        struct {                       \
            RZ__ARR_STRUCT_MEMBERS(T); \
        }

///    void rz_arr_free(RZ_Array(T) *da);
#    define rz_arr_free(da)     do { rz_free((da)->allocator, (da)->data, (da)->capacity); (da)->capacity = 0; (da)->len = 0; (da)->data = NULL; } while(0)
#    define rz_arr_dealloc(da)  rz_arr_free(da)

#    define rz_arr_clear(da)    ((da)->len = 0)

#    define rz_arr_clone(da)                               ((RZ_TYPEOF(*da)){.data = rz_memdup((da)->allocator, (da)->data, (da)->len * sizeof(*(da)->data)), .len = (da)->len, .capacity = (da)->len, .allocator = (da)->allocator})
#    define rz_arr_clone_with_allocator(da, _allocator)    ((RZ_TYPEOF(*da)){.data = rz_memdup(_allocator, (da)->data, (da)->len * sizeof(*(da)->data)), .len = (da)->len, .capacity = (da)->len, .allocator = _allocator})

///  Free memory for the array.
///  the second argument is ... for ease of use. for example: rz_arr_append(&da, (Struct){.a = a, .b = c, etc})
///    void rz_arr_append(RZ_Array(T) *da, T item);
#    define rz_arr_append(da, ...) do { rz__arr_grow(da, (da)->len + 1); (da)->data[(da)->len++] = __VA_ARGS__; } while (0)
#    define rz_arr_push rz_arr_append

///  Append several items to a dynamic array
///    void rz_arr_append_many(RZ_Array(T) *da, T *new_data, rz_usize new_data_len);
#    define rz_arr_append_many(da, new_data, new_data_len)  do { RZ_STATIC_ASSERT_TYPE_COMPATIBLE(*(da)->data, *new_data); rz_usize len = (new_data_len); rz__arr_grow(da, (da)->len + len); rz_memcpy((da)->data + (da)->len, (new_data), len * sizeof(*(da)->data)); (da)->len += len; } while (0)

///  Reserve size (reallocate data to the new capacity)
///    void rz_arr_reserve(RZ_Array(T) *da, rz_usize capacity);
#    define rz_arr_reserve(da, capacity)    rz__arr_grow(da, capacity)

///  resize the arrtor
///    void rz_arr_resize(RZ_Array(T) *da, rz_usize new_size);
#    define rz_arr_resize(da, new_size)     do { rz__arr_grow(da, new_size); (da)->len = (new_size); } while (0)

///  pop the last item in the arrtor. asserts if the arr is empty
///       T rz_arr_pop(RZ_Array(T) *da);
#    define rz_arr_pop(da)                  (da)->data[(RZ_ASSERT((da)->len > 0, "try to pop empty array/arrtor"), --(da)->len)]

///  remove the item (from param index `ì`)
///      T* rz_arr_pop(RZ_Array(T) *da, rz_usize i);
#    define rz_arr_remove(da, i)            *((RZ_TYPEOF((da)->data))rz__arr_remove((da)->data, &(da)->len, sizeof(*(da)->data), (RZ_ASSERT((i) < (da)->len, "try to remoove invalid index or empty arrtor"), (i))))

///  unordered remove the item (from param index `ì`)
///  delete item that not preserve order of data
///      T* rz_arr_remove_unordered(RZ_Array(T) *da, rz_usize i);
#    define rz_arr_remove_unordered(da, i)  (rz_memswap(&rz_arr_at(da, i), &rz_arr_last(da), sizeof(*(da)->data)), rz_arr_pop(da))

///  Macro that start with rz_arr is Generic macro that accept 
///     ArrayLike: DA_Array(T) and DA_ArrayView(T), that have `len` and `data`

///  Check if array is empty.
///    bool rz_arr_is_empty(ArrayLike *a);
#    define rz_arr_is_empty(a)           ((a)->len == 0 || (a)->data == NULL)

///  get first element from array. crash if array is empty.
///      T  rz_arr_first(ArrayLike<T> *a);
#    define rz_arr_first(a)              (a)->data[(RZ_ASSERT((a)->len > 0), 0)]
#    define rz_arr_first_checked(a)      rz_arr_get(a, 0)
#    define rz_arr_begin(a)              (a)->data

///  get last element from array. crash if array is empty.
///      T  rz_arr_last(ArrayLike<T> *a);
#    define rz_arr_last(a)               (a)->data[(RZ_ASSERT((a)->len > 0), (a)->len - 1)]
#    define rz_arr_last_checked(a)       rz_arr_get(a, (a)->len - 1)
#    define rz_arr_end(a)                ((a)->data + (a)->len)

#    define rz_arr_truncate(a, _len)     do { rz_usize n = _len; if (n <= (a)->len) (a)->len = n; } while(0)

///  get element form idx from array. crash if array idx index overflow.
///      T  rz_arr_at(ArrayLike<T> *a, rz_usize idx);
#    define rz_arr_at(a, idx)           (a)->data[(RZ_ASSERT((a)->len > idx), idx)]

///  get pointer of element form idx from array. 
///  if idx is invalid or array is empty, return NULL
///      T  rz_arr_get(ArrayLike<T> *a, rz_usize idx);
#    define rz_arr_get(a, idx)          (((idx) < (a)->len) ? &(a)->data[idx] : NULL)

///  get slice of array. return view of the type T (inclusive)
///  - start and end is index.
///
///  0,1,2,3,4
///      2   4
///      start
///          end
///  data = a->data + start
///  len  = (start - end) + 1
///  RZ_ArrayView<T> rz_arr_set(ArrayLike<T> *a, rz_usize start, rz_usize end);
#    define rz_arr_view(view_type, a, start, end)  ((view_type){ .len = (RZ_ASSERT(((start) < (a)->len) && ((end) < (a)->len) && ((start) <= (end))), ((end) - (start) + 1)), .data = (a)->data + (start) })

#    define rz_arr_reverse(a)            do { if ((a)->len > 1) { rz_usize half_len = (a)->len / 2; for (rz_usize i = 0; i < half_len; ++i) { RZ_SWAP((a)->data[i], (a)->data[((len - 1) - i)]); } } } while (0)

///  swap array item for idx_a and idx_b. return true if success. false if failed.
///  failed when the idx_a or idx_b is invalid or the arrtor is empty
///  bool   rz_arr_swap(ArrayLike<T> *a, rz_usize idx_a, rz_usize idx_b);
#    define rz_arr_swap(a, idx_a, idx_b)   rz_memswap(rz_arr_get(a, idx_a), rz_arr_get(a, idx_b), sizeof(*(a)->data));

///  iterate item from array.
///  example usage:
///     RZ_Array(int) arrays = example_function_return_array();
///     RZ_ArrayView(int) arrays = example_function_return_array_view();
///
///     rz_arr_foreach(it_item, &arrays) {
///         printf("item: %d", *it_item);
///     }
#    define rz_arr_foreach(it, da) for (RZ_TYPEOF((da)->data) it = (da)->data; it < ((da)->data + (da)->len); ++it)
#    define rz_foreach             rz_arr_foreach

///  iterate item from array with the index iterator.
///  this is different with rz_arr_foreach used.
///  example usage:
///     RZ_Array(int) arrays = example_function_return_slice();
///     RZ_ArrayView(int) arrays = example_function_return_slice();
///
///     rz_arr_foreach_idx(int, i, it_item, &arrays, {
///         printf("item[%zu]: %d", i, *it_item);
///     });
#    define rz_arr_foreach_idx(i, it, da, ...) do { RZ_TYPEOF((da)->data) it = (da)->data; for (rz_usize i = 0; i < (da)->len; i++, it++) __VA_ARGS__ } while (0)
#    define rz_foreach_idx                rz_arr_foreach_idx

#    define RZ_ARR_FIND_NOTFOUND          ((rz_usize)(-1))
#    define RZ_NOT_FOUND                  RZ_ARR_FIND_NOTFOUND

///  find item from start of array.
///  rz_usize rz_arr_find(ArrayLike<T> *a, T item);
#    define rz_arr_find(a, needle)               rz__arr_find   ((const RZ_ArrayViewOpaque *)(a), RZ_ADDRESSOF(*(a)->data, (needle)), sizeof(*(a)->data))
///  rz_usize rz_arr_find_by(ArrayLike<T> *a, bool(*pat_fn)(const void*item, const void *data), T pat_data);
#    define rz_arr_find_by(a, pat_fn, pat_data)  rz__arr_find_by((const RZ_ArrayViewOpaque *)(a), pat_fn, RZ_ADDRESSOF(*(a)->data, (pat_data)), sizeof(*(a)->data))

///  find item from end of array. (reserve)
///  rz_usize rz_arr_rfind(ArrayLike<T> *a, T item);
#    define rz_arr_rfind(a, needle)              rz__arr_rfind   ((const RZ_ArrayViewOpaque *)(a), RZ_ADDRESSOF(*(a)->data, (needle)), sizeof(*(a)->data))
///  rz_usize rz_arr_rfind_by(ArrayLike<T> *a, bool(*pat_fn)(const void*item, const void *data), T pat_data);
#    define rz_arr_rfind_by(a, pat_fn, pat_data) rz__arr_rfind_by((const RZ_ArrayViewOpaque *)(a), pat_fn, RZ_ADDRESSOF(*(a)->data, (pat_data)), sizeof(*(a)->data))

///  check if array is containing item.
///  bool   rz_arr_contains(ArrayLike<T> *a, T item);
#    define rz_arr_contains(a, needle)              (RZ_ARR_FIND_NOTFOUND != rz_arr_find(a, needle))
///  bool   rz_arr_contains_by(ArrayLike<T> *a, bool(*pat_fn)(const void*item, const void *data), T pat_data);
#    define rz_arr_contains_by(a, pat_fn, pat_data) (RZ_ARR_FIND_NOTFOUND != rz_arr_find_by(a, pat_fn, pat_data))

///  check if array is ends with other array (`hasystack` has suffix of `needle`)
///  bool   rz_arr_ends_with(ArrayLike<T> *haystack, ArrayLike<T> *needle);
#    define rz_arr_ends_with(a, needles)      ((needles).len <= (a)->len) && rz_memcmp((a)->data + ((a)->len - (needles)->len), (needles)->data, (needles)->len * sizeof(*(a)->data))
#    define rz_arr_ends_with_item(a, needle)  (1 <= (a)->len)             && rz_memcmp((a)->data + ((a)->len - 1), RZ_ADDRESSOF(RZ_TYPEOF(needle), needle), sizeof(needle))

///  check if array is starts with other array (`hasystack` has prefix of `needle`)
///  bool   rz_arr_starts_with(ArrayLike<T> *haystack, ArrayLike<T> *needle);
#    define rz_arr_starts_with(a, needles)     ((needles).len <= (a)->len) && rz_memcmp((a)->data, (needles)->data, (needles)->len * sizeof(*(a)->data))
#    define rz_arr_starts_with_item(a, needle) (1 <= (a)->len)             && rz_memcmp((a)->data, RZ_ADDRESSOF(RZ_TYPEOF(needle), needle), sizeof(needle))

///  split array into 2 result by n (mid). first_result and last_result. (inclusive)
///  the left   will contains [first .. mid]. excluding the mid. and
///  the right  will contains [mid .. end].
///
///  void   rz_arr_split_to_at(const ArrayLike<T> *a, rz_usize mid, ArrayLike<T> first_result, ArrayLike<T> last_result);
#    define rz_arr_split_at(a, at, left_result, right_result) do {\
        rz_usize mid = (at);                                                                 \
        if (mid <= (a)->len) {                                                               \
            *(left_result)  = rz_arr_view(RZ_TYPEOF(*(left_result)), a, 0, mid - 1);             \
            *(right_result) = rz_arr_view(RZ_TYPEOF(*(right_result)), a, mid, (a)->len - 1); \
        }                                                                                    \
    } while(0)

///  split array into 2 result by n (mid). first_result and last_result. (exclusive)
///  the left   will contains [first .. mid]. excluding the mid. and
///  the right  will contains [(mid + 1) .. end] excluding the mid to end.
///
///  void   rz_arr_split_to_at(const ArrayLike<T> *a, rz_usize mid, ArrayLike<T> first_result, ArrayLike<T> last_result);
#    define rz_arr_split_exclusive_at(a, at, left_result, right_result) do {\
        rz_usize mid = (at);                                                                 \
        if (mid <= (a)->len) {                                                               \
            *(left_result)  = rz_arr_view(RZ_TYPEOF(*(left_result)), a, 0, mid);             \
            *(right_result) = rz_arr_view(RZ_TYPEOF(*(right_result)),a, mid + 1, (a)->len - 1); \
        }                                                                                    \
    } while(0)


///  bool  rz_arr_split(ArrayLike<T> *a, T needle, ArrayLike<T> *result);
#    define rz_arr_split(a, needle, result)           rz__arr_split((RZ_ArrayViewOpaque *)(a), sizeof(*(a)->data), RZ_ADDRESSOF(*(a)->data, needle), (RZ_ArrayViewOpaque *)(result), false)
///  bool  rz_arr_split_inclusive(ArrayLike<T> *a, T needle, ArrayLike<T> *result);
#    define rz_arr_split_inclusive(a, needle, result) rz__arr_split((RZ_ArrayViewOpaque *)(a), sizeof(*(a)->data), RZ_ADDRESSOF(*(a)->data, needle), (RZ_ArrayViewOpaque *)(result), true)

#    define rz_arr_split_by(a, pat_fn, pat_data, result)           rz__arr_split_by((RZ_ArrayViewOpaque *)(a), sizeof(*(a)->data), pat_fn, RZ_ADDRESSOF(*(a)->data, pat_data), (RZ_ArrayViewOpaque *)(result), false)
#    define rz_arr_split_inclusive_by(a, pat_fn, pat_data, result) rz__arr_split_by((RZ_ArrayViewOpaque *)(a), sizeof(*(a)->data), pat_fn, RZ_ADDRESSOF(*(a)->data, pat_data), (RZ_ArrayViewOpaque *)(result), true)

///  bool   rz_arr_rsplit(ArrayLike<T> *a, T needle, ArrayLike<T> *result);
#    define rz_arr_rsplit(a, needle, result)           rz__arr_rsplit((RZ_ArrayViewOpaque *)(a), sizeof(*(a)->data), RZ_ADDRESSOF(*(a)->data, needle), (RZ_ArrayViewOpaque *)(result), false)
///  bool   rz_arr_rsplit_inclusive(ArrayLike<T> *a, T needle, ArrayLike<T> *result);
#    define rz_arr_rsplit_inclusive(a, needle, result) rz__arr_rsplit((RZ_ArrayViewOpaque *)(a), sizeof(*(a)->data), RZ_ADDRESSOF(*(a)->data, needle), (RZ_ArrayViewOpaque *)(result), true)

#    define rz_arr_rsplit_by(a, pat_fn, pat_data, result)           rz__arr_rsplit_by((RZ_ArrayViewOpaque *)(a), sizeof(*(a)->data), pat_fn, RZ_ADDRESSOF(*(a)->data, pat_data), (RZ_ArrayViewOpaque *)(result), false)
#    define rz_arr_rsplit_inclusive_by(a, pat_fn, pat_data, result) rz__arr_rsplit_by((RZ_ArrayViewOpaque *)(a), sizeof(*(a)->data), pat_fn, RZ_ADDRESSOF(*(a)->data, pat_data), (RZ_ArrayViewOpaque *)(result), true)

///  binary search array. 
///  rz_usize rz_arr_bsearch(ArrayLike<T> *a, T needle,  int(*cmpfunc)(void const *, void const *));
#    define rz_arr_bsearch(a, needle, cmpfunc)   rz__arr_bsearch((a)->data, (a)->len, sizeof(*(a)->data), RZ_ADDRESSOF(*(a)->data, needle), cmpfunc)

///  binary search array.( using stdlib qsort. TODO: implement our own sorting algoritm? )
///  void   rz_arr_sort(ArrayLike<T> *a, int(*cmpfunc)(void const *, void const *));
#    define rz_arr_qsort(a, cmpfunc)             qsort((a)->data, (a)->len, sizeof(*(a)->data), cmpfunc)

#    define rz__arr_grow(da, new_capacity) rz__arr_grow_impl((void **)&(da)->data, &(da)->capacity, sizeof(*(da)->data), (new_capacity), (da)->allocator)

///////////////
/// Hm (HashMap) & Hs (HashSet) Macors helpers
///
#    if defined(RZ_BIT64)
#        define RZ_SIPHASH_2_4
#    endif
#    ifdef RZ_SIPHASH_2_4
#        define RZ_SIPHASH_C_ROUNDS 2
#        define RZ_SIPHASH_D_ROUNDS 4
#    endif

#    ifndef RZ_SIPHASH_C_ROUNDS
#        define RZ_SIPHASH_C_ROUNDS 1
#    endif
#    ifndef RZ_SIPHASH_D_ROUNDS
#        define RZ_SIPHASH_D_ROUNDS 1
#    endif

#    ifndef RZ_HM_DEFAULT_CAPACITY
#        define RZ_HM_DEFAULT_CAPACITY 32U
#    endif /* ifndef RZ_HM_DEFAULT_CAPACITY */

#    ifndef RZ_HM_LOAD_FACTOR_PERCENT
#        define RZ_HM_LOAD_FACTOR_PERCENT 70U
#    endif /* ifndef RZ_HM_LOAD_FACTOR_PERCENT */

#    define RZ_HM_INDEX_DEFAULT ((rz_usize)(-1))

/// RZ__HM_STRUCT_MEMBERS(T)
/// 
/// Expandable block of struct members used by both RZ_Hm and RZ_Hs macros.
/// 
/// Parameters:
///  - T : the element type stored in `.data` (for maps: typically a struct
///         containing both key and value; for sets: the key type itself).
/// 
/// Expands to three members:
///  - rz_usize len;   -> number of unique items currently stored in the table.
///  - T *data;        -> pointer to an array of `T` elements managed by the
///                       underlying hash table implementation.
///  - rz_ptrdiff __temp;
///                    -> scratch/index member used by macros to return an
///                       index or temporary value across macro calls.
/// 
/// Notes / semantics:
///  - `.len` holds the count of unique entries. It is not a capacity field.
///  - `.data` points to the element storage; elements are laid out as `T[]`.
///  - `.__temp` is an implementation detail used by the macro wrappers to
///    communicate the index of a recently-found/inserted element. Do not
///    rely on its value outside the provided macro idioms.
/// 
#    define RZ__HM_STRUCT_MEMBERS(T)                            \
        /* .len - amount of unique items in the Hash Table. */  \
        rz_usize len;                                           \
        /* .data - amount of unique items in the Hash Table. */ \
        T            *data;                                     \
        rz_ptrdiff    __temp


/// ---- Example usage (illustrative) ----
/// 
/// // Map example
/// RZ_Hm(int, const char *) mymap = {0};
/// rz_hm_init(&mymap, .initial_capacity = 32);
/// rz_hm_put(&mymap, 42, "answer"); // _key used to locate slot, _value is full element
/// 
/// // Get a pointer to value for mutation:
/// const char **valptr = rz_hm_get(&mymap, 42);
/// if (valptr) *valptr = "new value"; 
/// 
/// // Free
/// rz_hm_free(&mymap);
/// 
/// // Set example
/// RZ_Hs(int) myset = {0};
/// rz_hs_init(&myset);
/// rz_hs_put(&myset, 7); // inserts integer 7
/// rz_hs_free(&myset);
/// 


/// RZ_Hm(Key, Value)
/// 
/// Declare an inline hash map struct type that stores pairs {Key, Value}.
/// 
/// Usage:
///  - Use directly as the type of a variable: `RZ_Hm(int, char *) mymap = {0};`
///  - Use typedef. example: `typedef RZ_Hm(Key, Value) MyHmAlias;` 
///  - The underlying `.data` elements are `struct { Key key; Value value; }`.
///  - The variable layout matches RZ_HmOpaque for `.len` and `.data` offsets.
/// 
/// Notes:
///  - This macro declares an anonymous struct type; use `RZ_TYPEOF` or
///    declare variables directly with it, or `typedef` first.
///  - The address-of semantics: the codebase uses helper macros that may take
///    addresses of key fields; ensure keys are addressable when used with
///    rvalues or temporary expressions.
/// 
#    define RZ_Hm(Key, Value)              \
        struct {                           \
            RZ__HM_STRUCT_MEMBERS(struct { \
                Key   key;                 \
                Value value;               \
            });                            \
        }

/// RZ_Hs(Key)
/// 
/// Declare an inline hash set struct type that stores unique Key elements.
/// 
/// Usage:
///  - `RZ_Hs(int) myset = {0};`  (stores ints)
///  - `typedef RZ_Hs(int) RZ_HsInt;`  create alias (stores ints)
///  - The underlying `.data` elements are `Key` values (not key/value pairs).
/// 
/// Notes:
///  - The same backing layout is used as for maps; thus this type is also
///    compatible with the opaque conversions asserted elsewhere.
/// 
#    define RZ_Hs(Key)                  \
        struct {                        \
            RZ__HM_STRUCT_MEMBERS(Key); \
        }

/// RZ__HM_IS_OPAQUE_CONVARTIBLE(hm_t) (internal use)
/// 
/// Compile-time assertion that the user-visible hash-map/set type `hm_t`
/// is layout-compatible with the internal RZ_HmOpaque type in terms of
/// the offsets of `len` and `data`.
/// 
/// Motivation:
///  - Several macro wrappers cast user types to `RZ_HmOpaque *` to call the
///    underlying runtime functions. To be safe, the offsets of `len` and
///    `data` must match between the user type and RZ_HmOpaque.
/// 
/// Behavior:
///  - Expands to a static assert (RZ_STATIC_ASSERT) checking:
///        offsetof(hm_t, len) == offsetof(RZ_HmOpaque, len) &&
///        offsetof(hm_t, data) == offsetof(RZ_HmOpaque, data)
///  - Produces a compile-time error with a message if the assertion fails.
/// 
/// Notes:
///  - This is invoked after calls to the runtime wrappers to ensure the
///    cast used in the call was safe.
/// 
#    define RZ__HM_IS_OPAQUE_CONVARTIBLE(hm_t)  RZ_STATIC_ASSERT(((RZ_OFFSETOF(hm_t, len) == RZ_OFFSETOF(RZ_HmOpaque, len)) && (RZ_OFFSETOF(hm_t, data) == RZ_OFFSETOF(RZ_HmOpaque, data))), #hm_t " not convertable to RZ_HmOpaque")
/// rz__hmkeyvalue(typekv, _key) (internal use)
/// Build an RZ__HmKeyValue temporary describing the key used in the call.
#    define rz__hmkeyvalue(typekv, _key)        ((RZ__HmKeyValue){.key = RZ_ADDRESSOF((typekv)->key, _key), .keysize = sizeof(_key), .valueoffs = RZ_OFFSETOF(RZ_TYPEOF(*typekv), value)})
/// rz__hm_call(fn, hm, _key) (internal use)
/// helper for calling hm function
#    define rz__hm_call(fn, hm, _key)           rz__hm_##fn((RZ_HmOpaque *)hm, sizeof(*(hm)->data), rz__hmkeyvalue((hm)->data, (_key)))

/* ---- public API macros for maps and sets ---- */

/// void rz_hm_init(RZ_Hm(Key, Value) *hm, RZ__HmInitOpt...)
/// 
/// Initialize a hash map variable `hm`. The macro forwards to
/// rz__hm_init((RZ_HmOpaque *)hm, RZ__HmInitOpt{ .elemsize = sizeof(*(hm)->data), ... })
/// 
/// Example:
///  RZ_Hm(int, const char *) mymap = {0};
///  rz_hm_init(&mymap, .initial_capacity = 64);
///
///  typedef RZ_Hm(int, const char *) MapInt;
///  MapInt mymap = {0};
///  rz_hm_init(&mymap, .initial_capacity = 64);
/// 
#    define rz_hm_init(hm, ...)      rz__hm_init((RZ_HmOpaque *)hm, (RZ__HmInitOpt){ .elemsize = sizeof(*(hm)->data) __VA_OPT__(, ) __VA_ARGS__ }); RZ__HM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*hm))

/// void rz_hm_free(RZ_Hm(Key, Value) *hm)
/// 
/// Free resources allocated by the underlying hash-map implementation for `hm`.
/// Forwards to rz__hm_free((RZ_HmOpaque *)hm, sizeof(*(hm)->data)).
/// 
/// Example:
///  rz_hm_free(&mymap);
/// 
#    define rz_hm_free(hm)           rz__hm_free((RZ_HmOpaque *)hm, sizeof(*(hm)->data)); RZ__HM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*hm))

/// void rz_hm_reset(RZ_Hm(Key, Value) *hm)
/// 
/// Reset the hash map to an empty state but keep allocated resources if the
/// implementation supports it (semantic depends on rz__hm_reset).
/// 
/// Example:
///  rz_hm_reset(&mymap); // mymap.len -> 0, storage may be reused
/// 
#    define rz_hm_reset(hm)          rz__hm_reset((RZ_HmOpaque *)hm, sizeof(*(hm)->data)); RZ__HM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*hm))


/// rz_ptrdiff? rz_hm_find(RZ_Hm(Key, Value) *hm, Key _key)
/// 
/// Find an element by key. 
/// Returns `rz_ptrdiff` as index to `hm->data`. if key is not found. return -1
/// 
#    define rz_hm_find(hm, _key)               rz__hm_call(find,         hm, _key); RZ__HM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*hm))

/// rz_ptrdiff rz_hm_find_default(RZ_Hm(Key, Value) *hm, Key _key)
/// 
/// Variant of find that inserted new with default value if not found.
/// Return `rz_ptrdiff` guaranteed valid index.
/// 
#    define rz_hm_find_default(hm, _key)       rz__hm_call(find_default, hm, _key); RZ__HM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*hm))

/// void rz_hm_delete(RZ_Hm(Key, Value) *hm, Key _key)
/// 
/// Delete the entry identified by _key (if present). The macro forwards
/// to the underlying delete implementation.
/// 
#    define rz_hm_delete(hm, key)              rz__hm_call(delete,       hm, _key); RZ__HM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*hm))

/// Value *rz_hm_get(RZ_Hm(Key, Value) *hm, Key _key)
/// 
/// Get (create-if-missing or find-and-return) a pointer to the value for key `_key`. 
/// 
/// Example:
///  Value *v = rz_hm_get(&mymap, some_key);
///  *v = new_value;
/// 
#    define rz_hm_get(hm, _key)          (hm = rz__hm_call(get, hm, _key), &(hm)->data[(hm)->__temp].value);   RZ__HM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*hm))
#    define rz_hm_find_get(hm, _key)     ((void)rz__hm_call(find, hm, _key), ((hm)->__temp == -1) ? NULL : &(hm)->data[(hm)->__temp].value);   RZ__HM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*hm))

/// bool rz_hm_put(RZ_Hm(Key, Value) hm, Key _key, Value _value)
/// 
/// Insert or update an element.
/// 
/// Example (map):
///  bool true_if_insert_new = rz_hm_put(&mymap, key_expr, value_expr);
/// 
/// Pitfalls:
///  - If you intend to set only the `value` member for an existing key, use
///    rz_hm_get and write to the `value` field directly.
/// 
#    define rz_hm_put(hm, _key, _value)  ((void)rz__hm_call(put,          hm, _key), (hm)->data[(hm)->__temp].value = _value); RZ__HM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*hm))

/// ---- convenience alias macros for sets (RZ_Hs) ----
/// 
/// For sets, the same operations are used; we alias the hm macros to hs macros.
/// This makes the API symmetric for users.
/// 
#    define rz_hs_init               rz_hm_init
#    define rz_hs_free               rz_hm_free
#    define rz_hs_reset              rz_hm_reset
#    define rz_hs_find               rz_hm_find
#    define rz_hs_find_default       rz_hm_find_default
#    define rz_hs_delete             rz_hm_delete
#    define rz_hs_get                rz_hm_get
#    define rz_hs_put(hm, _key)      { rz__hm_call(put, hm, _key); RZ__HM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*hm)); } while(0)


#    define rz_hm_foreach rz_foreach

///////////////
/// Bm (BtreeMap) and Bs (BtreeSet) Macors helpers
///
#    define RZ_Bm(Key, Value) struct { struct { Key key; Value value; } **__temp_data; }
#    define RZ_Bs(Key)        struct { Key **__temp_data; }

#    define rz__bmkeyvalue(typekv, _key, _value)    ((RZ__BmKeyValue){.key = RZ_ADDRESSOF((typekv)->key, _key), .keysize sizeof(_key), .value = _value, .valuesize = sizeof(_value), .valueoffs = RZ_OFFSETOF(RZ_TYPEOF(typekv), value)})
#    define rz__bmkey(typekv, _key)                 ((RZ__BmKeyValue){.key = RZ_ADDRESSOF((typekv)->key, _key), .keysize sizeof(key), .valueoffs = RZ_OFFSETOF(RZ_TYPEOF(typekv), value)})

#    define RZ__BM_IS_OPAQUE_CONVARTIBLE(bm_type)  RZ_STATIC_ASSERT(((RZ_OFFSETOF(bm_type, len)      == RZ_OFFSETOF(RZ_BmOpaque, len)) && (RZ_OFFSETOF(bm_type, data)     == RZ_OFFSETOF(RZ_BmOpaque, data)))), #bm_type " not convertable to RZ_BmOpaque")

#    define rz_bm_init(bm, ...)             rz__bm_init        ((RZ_BmOpaque *)bm, (RZ__BmInitOpt){ .elemsize = sizeof(*(bm)->data) __VA_OPT__(, ) __VA_ARGS__ }); RZ__BM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*bm))
#    define rz_bm_free(bm)                  rz__bm_free        ((RZ_BmOpaque *)bm, sizeof(*(bm)->data)); RZ__BM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*bm))
#    define rz_bm_reset(bm)                 rz__bm_reset       ((RZ_BmOpaque *)bm, sizeof(*(bm)->data)); RZ__BM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*bm))
#    define rz_bm_put(bm, _key, _value)     (bm = rz__bm_put   ((RZ_BmOpaque *)bm, sizeof(*(bm)->data), rz__bmkeyvalue((bm)->data, _key)), (bm)->data[(bm)->__temp] = _value;); RZ__BM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*bm))
#    define rz_bm_find(bm, _key)            rz__bm_find        ((RZ_BmOpaque *)bm, sizeof(*(bm)->data), rz__bmkeyvalue((bm)->data, _key)); RZ__BM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*bm))
#    define rz_bm_find_default(bm, _key)    rz__bm_find_default((RZ_BmOpaque *)bm, sizeof(*(bm)->data), rz__bmkeyvalue((bm)->data, _key)); RZ__BM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*bm))
#    define rz_bm_get(bm, _key)             (bm = rz__bm_get   ((RZ_BmOpaque *)bm, sizeof(*(bm)->data), rz__bmkeyvalue((bm)->data, _key)), &(bm)->data[(bm)->__temp].value); RZ__BM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*bm))
#    define rz_bm_delete(bm, key)           rz__bm_delete((RZ_BmOpaque *)bm, sizeof(*(bm)->data), rz__bmkeyvalue((bm)->data, _key)); RZ__BM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*bm))

#    define rz_bs_init                      rz_bm_init
#    define rz_bs_free                      rz_bm_free
#    define rz_bs_reset                     rz_bm_reset
#    define rz_bs_put(bm, _key)             { rz__bm_put( (RZ_BmOpaque *)bm, sizeof(*(*((bm)->__temp_data))), rz__bmkeyvalue(*((bm)->__temp_data, _key, NULL)); RZ__BM_IS_OPAQUE_CONVARTIBLE(RZ_TYPEOF(*bm)) } while(0)
#    define rz_bs_find                      rz_bm_find
#    define rz_bs_find_default              rz_bm_find_default
#    define rz_bs_get                       rz_bm_get
#    define rz_bs_delete                    rz_bm_delete

// clang-format on

#    ifdef __cplusplus
extern "C" {
#    endif

///////////////
/// Array & Vec Imlementation details
///
typedef RZ_Array(void) RZ_ArrayOpaque;
typedef RZ_ArrayView(void) RZ_ArrayViewOpaque;
typedef bool (*RZ__ArrFindPatternFn)(const void *item, const void *data);

RZ_DEC void  rz__arr_grow_impl(void **data, rz_usize *capacity, rz_usize elemsize, rz_usize new_capacity, RZ_Allocator allocator);
RZ_DEC void *rz__arr_remove(void *data, rz_usize *len, rz_usize type_size, rz_usize idx);

// RZ_DEC rz_usize rz__slice_rfind(RZ_ArrayViewOpaque *s, rz_usize type_size, void *item);

RZ_DEC rz_usize rz__arr_find(const RZ_ArrayViewOpaque *arr, void const *item, rz_usize elemsize);
RZ_DEC rz_usize rz__arr_rfind(const RZ_ArrayViewOpaque *arr, void const *item, rz_usize elemsize);
RZ_DEC rz_usize rz__arr_find_by(const RZ_ArrayViewOpaque *arr, RZ__ArrFindPatternFn pat, void const *pat_data, rz_usize elemsize);
RZ_DEC rz_usize rz__arr_rfind_by(const RZ_ArrayViewOpaque *arr, RZ__ArrFindPatternFn pat, void const *pat_data, rz_usize elemsize);

RZ_DEC rz_usize rz__arr_bsearch(const RZ_ArrayViewOpaque *data, rz_usize elemsize, void const *needle, int (*cmpfunc)(void const *, void const *));

RZ_DEC bool rz__arr_split(RZ_ArrayViewOpaque *arr, rz_usize elemsize, void const *needle, RZ_ArrayViewOpaque *res, bool inclusive);
RZ_DEC bool rz__arr_rsplit(RZ_ArrayViewOpaque *arr, rz_usize elemsize, void const *needle, RZ_ArrayViewOpaque *res, bool inclusive);
RZ_DEC bool rz__arr_split_by(RZ_ArrayViewOpaque *arr, rz_usize elemsize, bool (*pat)(const void *, const void *), void *pat_data, RZ_ArrayViewOpaque *res, bool inclusive);
RZ_DEC bool rz__arr_rsplit_by(RZ_ArrayViewOpaque *arr, rz_usize elemsize, bool (*pat)(const void *, const void *), void *pat_data, RZ_ArrayViewOpaque *res, bool inclusive);

///////////////
/// Hm (HashMap) & Hs (HashSet) Imlementation details
///
typedef RZ_Hs(void) RZ_HmOpaque;

typedef enum
{
    RZ_HM_HASHCMP_HASH,
    RZ_HM_HASHCMP_CMP,
} RZ_HmHashCmpOp;

typedef rz_usize (*RZ_HmHashCmpFn)(RZ_HmHashCmpOp op, void const *a, void const *b, rz_usize len, rz_usize seed);

typedef struct {
    rz_usize       elemsize;
    rz_usize       initial_capacity;
    RZ_HmHashCmpFn hashcmp;
    RZ_Allocator   allocator;
} RZ__HmInitOpt;

typedef struct {
    void    *key;
    rz_usize keysize;
    rz_usize valueoffs;
} RZ__HmKeyValue;

RZ_DEC void rz__hm_init(RZ_HmOpaque *opq, RZ__HmInitOpt opt);
RZ_DEC void rz__hm_free(RZ_HmOpaque *opq, rz_usize elemsize);
RZ_DEC void rz__hm_reset(RZ_HmOpaque *opq, rz_usize elemsize);

RZ_DEC void      *rz__hm_put(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv);
RZ_DEC rz_ptrdiff rz__hm_find(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv);
RZ_DEC rz_ptrdiff rz__hm_find_default(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv);
RZ_DEC bool       rz__hm_delete(RZ_HmOpaque *opq, rz_usize elemsize, RZ__HmKeyValue kv);

// for security against attackers, seed the library with a random number, at least time() but stronger is better
RZ_DEC void rz_rand_seed(rz_usize seed);

// these are the hash functions used internally if you want to test them or use them for other purposes
RZ_DEC rz_usize rz_hm_hasheq_bytes(RZ_HmHashCmpOp op, void const *a, void const *b, rz_usize len, rz_usize seed);
RZ_DEC rz_usize rz_hm_hasheq_string(RZ_HmHashCmpOp op, void const *a, void const *b, rz_usize len, rz_usize seed);

#    ifndef rz_hm_default_hash
#        define rz_hm_default_hash rz_hm_siphash_hash
#    endif

RZ_DEC rz_usize rz_hm_siphash_hash(void const *data, rz_usize size, rz_usize seed);
RZ_DEC rz_usize rz_hm_djb2_hash(void const *data, rz_usize size, rz_usize seed);
RZ_DEC rz_usize rz_hm_fnv1a_hash(void const *data, rz_usize size, rz_usize seed);
RZ_DEC rz_usize rz_hm_fnv1_hash(void const *data, rz_usize size, rz_usize seed);
RZ_DEC rz_usize rz_hm_sdbm_hash(void const *data, rz_usize size, rz_usize seed);
RZ_DEC rz_usize rz_hm_knuth_hash(void const *data, rz_usize size, rz_usize seed);
RZ_DEC rz_usize rz_hm_id_hash(void const *data, rz_usize size, rz_usize seed);

///////////////
/// Bm (BtreeMap) & Bs (BtreeSet) Imlementation details
///
typedef RZ_Bs(void) RZ_BmOpaque;
typedef rz_ptrdiff (*RZ_BmCmpFn)(void const *a, void const *b, rz_usize len);

typedef struct {
    rz_usize     elemsize;
    rz_usize     initial_capacity;
    RZ_BmCmpFn   cmp;
    RZ_Allocator allocator;
} RZ__BmInitOpt;

typedef struct {
    void    *key;
    void    *value;
    rz_usize keysize;
    rz_usize valuesize;
    rz_usize valueoffs;
} RZ__BmKeyValue;

RZ_DEC void rz__bm_init(RZ_BmOpaque *opq, RZ__BmInitOpt opt);
RZ_DEC void rz__bm_free(RZ_BmOpaque *opq, rz_usize elemsize);
RZ_DEC void rz__bm_reset(RZ_BmOpaque *opq, rz_usize elemsize);

RZ_DEC void      *rz__bm_put(RZ_BmOpaque *opq, rz_usize elemsize, RZ__BmKeyValue kv);
RZ_DEC rz_ptrdiff rz__bm_find(RZ_BmOpaque *opq, rz_usize elemsize, RZ__BmKeyValue kv);
RZ_DEC rz_ptrdiff rz__bm_find_default(RZ_BmOpaque *opq, rz_usize elemsize, RZ__BmKeyValue kv);
RZ_DEC bool       rz__bm_delete(RZ_BmOpaque *opq, rz_usize elemsize, RZ__BmKeyValue kv);

#    ifdef __cplusplus
}
#    endif
#endif /* end of include guard: RZ_HASHMAP_H */
