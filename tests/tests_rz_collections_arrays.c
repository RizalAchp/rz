#define RZ_TESTS_IMPL

#include "rz_common.h"
#include "rz_tests.h"

#include "rz_collections.h"
#include "tests_allocator.h"

RZ_TESTS_MAIN()

typedef RZ_ArrayView(rz_int) IntArrayView;
typedef RZ_Array(rz_int) IntArray;

#define to_slice(arr) ((IntArrayView){.data = (arr)->data, .len = (arr)->len})

#define helper_assert_arr_eq(slice, ...)                                                                  \
    do {                                                                                                  \
        RZ_TYPEOF(*(slice).data) arr[] = __VA_ARGS__;                                                     \
        RZ_TESTS_ASSERT_EQ((slice).len, RZ_ARRAY_LEN(arr));                                               \
        for (rz_usize i = 0; i < RZ_ARRAY_LEN(arr); ++i) { RZ_TESTS_ASSERT_EQ((slice).data[i], arr[i]); } \
    } while (0);

#define assert_arr_empty(arr)              \
    RZ_TESTS_ASSERT_EQ(arr->capacity, 0u); \
    RZ_TESTS_ASSERT_EQ(arr->len, 0u);      \
    RZ_TESTS_ASSERT_EQ(arr->data, NULL)

RZ_TESTS_SETUP(IntArray) {
    fixture->allocator = rz_test_allocator(rz_std_allocator());
    assert_arr_empty(fixture);
}

RZ_TESTS_TEARDOWN(IntArray) {
    rz_arr_free(fixture);
    assert_arr_empty(fixture);

    RZ_TESTS_ALLOCATOR_ASSERT_DEINIT(fixture->allocator);
}

RZ_TESTS(IntArray, vector_or_dynamic_array_push_pop) {
    rz_arr_append(fixture, 69);
    rz_arr_append(fixture, 420);

    RZ_TESTS_ASSERT_EQ(fixture->capacity, RZ_ARR_INIT_CAPACITY, "on first usage, the capacity of the vector should initialize in RZ_ARR_INIT_CAPACITY");
    RZ_TESTS_ASSERT_EQ(fixture->len, 2U);
    RZ_TESTS_ASSERT_NE(fixture->data, NULL);

    RZ_TESTS_ASSERT_EQ(rz_arr_pop(fixture), 420);
    RZ_TESTS_ASSERT_EQ(rz_arr_pop(fixture), 64);

    RZ_TESTS_ASSERT_EQ(fixture->capacity, RZ_ARR_INIT_CAPACITY);
    RZ_TESTS_ASSERT_EQ(fixture->len, 0U);
    RZ_TESTS_ASSERT_NE(fixture->data, NULL);
}

RZ_TESTS(IntArray, vector_or_dynamic_array_remove_swap) {
    rz_usize len = 10;
    for (rz_usize i = 0; i < len; ++i) { rz_arr_push(fixture, i); }
    RZ_TESTS_ASSERT_EQ(fixture->len, len);
    rz_i32 removed_idx_6 = rz_arr_remove_unordered(fixture, 6);
    rz_i32 removed_idx_7 = rz_arr_remove_unordered(fixture, 7);

    RZ_TESTS_ASSERT_EQ(removed_idx_6, 6);
    RZ_TESTS_ASSERT_EQ(removed_idx_7, 7);

    rz_i32 expected[] = {0, 1, 2, 3, 4, 5, 9, 8};
    for (rz_usize i = 0; i < RZ_ARRAY_LEN(expected); ++i) {
        //
        RZ_TESTS_ASSERT_EQ(fixture->data[i], expected[i]);
    }

    RZ_TESTS_ASSERT_EQ(rz_arr_remove_unordered(fixture, RZ_ARRAY_LEN(expected) - 1), expected[RZ_ARRAY_LEN(expected) - 1],
                       "if removed unordered / swap the last element, the order is ok");
    for (rz_usize i = 0; i < RZ_ARRAY_LEN(expected) - 1; ++i) {
        //
        RZ_TESTS_ASSERT_EQ(fixture->data[i], expected[i]);
    }
}

RZ_TESTS(IntArray, vector_or_dynamic_array_remove) {
    rz_usize len = 10;
    for (rz_usize i = 0; i < len; ++i) { rz_arr_push(fixture, i); }
    RZ_TESTS_ASSERT_EQ(fixture->len, len);
    rz_i32 removed_idx_6 = rz_arr_remove(fixture, 6);
    RZ_TESTS_ASSERT_EQ(fixture->len, 9u);
    RZ_TESTS_ASSERT_EQ(removed_idx_6, 6);
    rz_i32 removed_idx_7 = rz_arr_remove(fixture, 7);
    RZ_TESTS_ASSERT_EQ(fixture->len, 8u);
    RZ_TESTS_ASSERT_EQ(removed_idx_7, 7);

    rz_i32 expected[] = {0, 1, 2, 3, 4, 5, 8, 9};
    for (rz_usize i = 0; i < RZ_ARRAY_LEN(expected); ++i) {
        //
        RZ_TESTS_ASSERT_EQ(fixture->data[i], expected[i]);
    }
    RZ_TESTS_ASSERT_EQ(rz_arr_remove(fixture, 0), 0);
    RZ_TESTS_ASSERT_EQ(fixture->len, 7u);
    for (rz_usize i = 1; i < RZ_ARRAY_LEN(expected); ++i) {
        //
        RZ_TESTS_ASSERT_EQ(fixture->data[i], expected[i]);
    }
}

RZ_TESTS(IntArray, array_getters) {
    rz_usize len = 10;
    for (rz_usize i = 0; i < len; ++i) { rz_arr_push(fixture, i); }
    RZ_TESTS_ASSERT_EQ(fixture->len, len);
    RZ_TESTS_ASSERT_FALSE(rz_arr_is_empty(fixture));

    RZ_TESTS_ASSERT_EQ(rz_arr_first(fixture), 0, "get first element");
    RZ_TESTS_ASSERT_EQ(rz_arr_last(fixture), 9, "get last element");

    RZ_TESTS_ASSERT_EQ(rz_arr_begin(fixture), fixture->data, "get data pointer pointing to the begin of data");
    RZ_TESTS_ASSERT_EQ(rz_arr_end(fixture), (fixture->data + fixture->len), "get data pointer pointing to the end of data");

    RZ_TESTS_ASSERT_EQ(rz_arr_at(fixture, 2), 1);
    RZ_TESTS_ASSERT_EQ(rz_arr_at(fixture, 3), 2);
    RZ_TESTS_ASSERT_EQ(rz_arr_at(fixture, 7), 6);

    RZ_TESTS_ASSERT_EQ(rz_arr_get(fixture, 10), NULL, "try to get with invalid index");
    RZ_TESTS_ASSERT_EQ(rz_arr_get(fixture, 69), NULL, "try to get with invalid index");
    RZ_TESTS_ASSERT_EQ(rz_arr_get(fixture, 420), NULL, "try to get with invalid index");
}

RZ_TESTS(IntArray, array_reverse) {
    rz_usize len = 10;
    for (rz_usize i = 0; i < len; ++i) { rz_arr_push(fixture, i); }
    RZ_TESTS_ASSERT_EQ(fixture->len, len);
    RZ_TESTS_ASSERT_FALSE(rz_arr_is_empty(fixture));

    rz_arr_reverse(fixture);
    rz_i32 expected_reverse[] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    for (rz_usize i = 0; i < RZ_ARRAY_LEN(expected_reverse); ++i) {
        //
        RZ_TESTS_ASSERT_EQ(fixture->data[i], expected_reverse[i]);
    }
}

static int tests_int_cmp(const void *a, const void *b) {
    return *((const int *)a) - *((const int *)b);
}
static bool is_mod2(const void *item, const void *data) {
    RZ_UNUSED(data);
    return (*((const rz_int *)item) % 2) == 0;
}

RZ_TESTS(IntArray, array_queries) {
    rz_usize len = 10;
    for (rz_usize i = 0; i < len; ++i) { rz_arr_push(fixture, i); }
    RZ_TESTS_ASSERT_EQ(fixture->len, len);
    RZ_TESTS_ASSERT_FALSE(rz_arr_is_empty(fixture));

    RZ_TESTS_ASSERT_EQ(rz_arr_find(fixture, 6), 7u);
    RZ_TESTS_ASSERT_EQ(rz_arr_find(fixture, 9), 10u);
    RZ_TESTS_ASSERT_EQ(rz_arr_find(fixture, 69), RZ_ARR_FIND_NOTFOUND);
    RZ_TESTS_ASSERT_EQ(rz_arr_find(fixture, 420), RZ_ARR_FIND_NOTFOUND);

    RZ_TESTS_ASSERT_EQ(rz_arr_rfind(fixture, 6), 7u);
    RZ_TESTS_ASSERT_EQ(rz_arr_rfind(fixture, 9), 10u);
    RZ_TESTS_ASSERT_EQ(rz_arr_rfind(fixture, 69), RZ_ARR_FIND_NOTFOUND);
    RZ_TESTS_ASSERT_EQ(rz_arr_rfind(fixture, 420), RZ_ARR_FIND_NOTFOUND);

    RZ_TESTS_ASSERT_EQ(rz_arr_find_by(fixture, is_mod2, 0), 2u);
    RZ_TESTS_ASSERT_EQ(rz_arr_rfind_by(fixture, is_mod2, 0), 8u);

    RZ_TESTS_ASSERT_TRUE(rz_arr_contains(fixture, 6));
    RZ_TESTS_ASSERT_TRUE(rz_arr_contains(fixture, 7));
    RZ_TESTS_ASSERT_FALSE(rz_arr_contains(fixture, 69));
    RZ_TESTS_ASSERT_FALSE(rz_arr_contains(fixture, 420));

    RZ_TESTS_ASSERT_TRUE(rz_arr_starts_with_item(fixture, 0));
    RZ_TESTS_ASSERT_TRUE(rz_arr_ends_with_item(fixture, 9));
    RZ_TESTS_ASSERT_FALSE(rz_arr_starts_with_item(fixture, 69));
    RZ_TESTS_ASSERT_FALSE(rz_arr_ends_with_item(fixture, 420));

    RZ_TESTS_ASSERT_EQ(rz_arr_bsearch(fixture, 7, tests_int_cmp), 6u);
    RZ_TESTS_ASSERT_EQ(rz_arr_bsearch(fixture, 9, tests_int_cmp), 10u);
    RZ_TESTS_ASSERT_EQ(rz_arr_bsearch(fixture, 69, tests_int_cmp), RZ_ARR_FIND_NOTFOUND);
    RZ_TESTS_ASSERT_EQ(rz_arr_bsearch(fixture, 420, tests_int_cmp), RZ_ARR_FIND_NOTFOUND);
}

RZ_TESTS(IntArray, array_split_at_index) {
    assert_arr_empty(fixture);
    int      test_items[] = {1, 2, 3};
    rz_usize len          = RZ_ARRAY_LEN(test_items);
    for (rz_usize i = 0; i < len; ++i) { rz_arr_push(fixture, i); }
    RZ_TESTS_ASSERT_EQ(fixture->len, len);
    RZ_TESTS_ASSERT_FALSE(rz_arr_is_empty(fixture));

    {
        IntArrayView left  = {0};
        IntArrayView right = {0};
        rz_arr_split_to_at(fixture, 0, &left, &right);

        RZ_TESTS_ASSERT_EQ(left.len, 0u);
        helper_assert_arr_eq(right, {1, 2, 3});
    }

    {
        IntArrayView left  = {0};
        IntArrayView right = {0};
        rz_arr_split_to_at(fixture, 2, &left, &right);

        helper_assert_arr_eq(left, {1, 2});
        helper_assert_arr_eq(right, {3});
    }

    {
        IntArrayView left  = {0};
        IntArrayView right = {0};
        rz_arr_split_to_at(fixture, 3, &left, &right);

        helper_assert_arr_eq(left, {1, 2, 3});
        RZ_TESTS_ASSERT_EQ(right.len, 0u);
    }

    IntArrayView slice  = to_slice(fixture);
    IntArrayView result = {0};
    rz_arr_split_at(&slice, 2, &result);

    helper_assert_arr_eq(slice, {1, 2});
    helper_assert_arr_eq(result, {3});
}

bool is_mod_of_3(const void *item, const void *data) {
    RZ_UNUSED(data);
    return ((*(rz_int *)item) % 3) == 0;
}

RZ_TESTS(IntArray, array_split_rsplit_inclusive_exclusive) {
    assert_arr_empty(fixture);
    int      test_items[] = {10, 40, 33, 20};
    rz_usize len          = RZ_ARRAY_LEN(test_items);
    for (rz_usize i = 0; i < len; ++i) { rz_arr_push(fixture, i); }
    RZ_TESTS_ASSERT_EQ(fixture->len, len);
    RZ_TESTS_ASSERT_FALSE(rz_arr_is_empty(fixture));
    IntArrayView slice = {0};

    {
        // try to split empty slice/array
        IntArrayView res = {0};
        RZ_TESTS_ASSERT_FALSE(rz_arr_split(&slice, 33, &res));
        RZ_TESTS_ASSERT_FALSE(rz_arr_split_inclusive(&slice, 33, &res));
        RZ_TESTS_ASSERT_FALSE(rz_arr_split_by(&slice, is_mod_of_3, 0, &res));
        RZ_TESTS_ASSERT_FALSE(rz_arr_split_inclusive_by(&slice, is_mod_of_3, 0, &res));

        RZ_TESTS_ASSERT_FALSE(rz_arr_rsplit(&slice, 33, &res));
        RZ_TESTS_ASSERT_FALSE(rz_arr_rsplit_inclusive(&slice, 33, &res));
        RZ_TESTS_ASSERT_FALSE(rz_arr_rsplit_by(&slice, is_mod_of_3, 0, &res));
        RZ_TESTS_ASSERT_FALSE(rz_arr_rsplit_inclusive_by(&slice, is_mod_of_3, 0, &res));

        RZ_TESTS_ASSERT_TRUE(rz_arr_is_empty(&res));
    }
    {
        // try to split but the unknown item in container. move all items in slice to the result
        slice            = to_slice(fixture);
        IntArrayView res = {0};
        RZ_TESTS_ASSERT_FALSE(rz_arr_split(&slice, 69, &res));
        RZ_TESTS_ASSERT_TRUE(rz_arr_is_empty(&slice));
        helper_assert_arr_eq(res, {10, 40, 33, 20});

        slice = to_slice(fixture);
        res   = (IntArrayView){0};
        RZ_TESTS_ASSERT_FALSE(rz_arr_rsplit(&slice, 69, &res));
        RZ_TESTS_ASSERT_TRUE(rz_arr_is_empty(&slice));
        helper_assert_arr_eq(res, {10, 40, 33, 20});
    }

    {
        slice            = to_slice(fixture);
        IntArrayView res = {0};
        RZ_TESTS_ASSERT_TRUE(rz_arr_split(&slice, 33, &res));

        helper_assert_arr_eq(slice, {20});
        helper_assert_arr_eq(res, {10, 40});
    }
    {
        slice            = to_slice(fixture);
        IntArrayView res = {0};
        RZ_TESTS_ASSERT_TRUE(rz_arr_split_inclusive(&slice, 33, &res));

        helper_assert_arr_eq(slice, {20});
        helper_assert_arr_eq(res, {10, 40, 33});
    }
    {
        slice            = to_slice(fixture);
        IntArrayView res = {0};
        RZ_TESTS_ASSERT_TRUE(rz_arr_rsplit(&slice, 33, &res));

        helper_assert_arr_eq(slice, {10, 40});
        helper_assert_arr_eq(res, {20});
    }
    {
        slice            = to_slice(fixture);
        IntArrayView res = {0};
        RZ_TESTS_ASSERT_TRUE(rz_arr_rsplit_inclusive(&slice, 33, &res));

        helper_assert_arr_eq(slice, {10, 40});
        helper_assert_arr_eq(res, {33, 20});
    }

    {
        slice            = to_slice(fixture);
        IntArrayView res = {0};
        RZ_TESTS_ASSERT_TRUE(rz_arr_split_by(&slice, is_mod_of_3, 0, &res));

        helper_assert_arr_eq(slice, {20});
        helper_assert_arr_eq(res, {10, 40});
    }
    {
        slice            = to_slice(fixture);
        IntArrayView res = {0};
        RZ_TESTS_ASSERT_TRUE(rz_arr_split_inclusive_by(&slice, is_mod_of_3, 0, &res));

        helper_assert_arr_eq(slice, {20});
        helper_assert_arr_eq(res, {10, 40, 33});
    }
    {
        slice            = to_slice(fixture);
        IntArrayView res = {0};
        RZ_TESTS_ASSERT_TRUE(rz_arr_rsplit_by(&slice, is_mod_of_3, 0, &res));

        helper_assert_arr_eq(slice, {10, 40});
        helper_assert_arr_eq(res, {20});
    }
    {
        slice            = to_slice(fixture);
        IntArrayView res = {0};
        RZ_TESTS_ASSERT_TRUE(rz_arr_rsplit_inclusive_by(&slice, is_mod_of_3, 0, &res));

        helper_assert_arr_eq(slice, {10, 40});
        helper_assert_arr_eq(res, {33, 20});
    }
}

RZ_TESTS(IntArray, array_strip) {
    rz_usize len = 10;
    for (rz_usize i = 0; i < len; ++i) { rz_arr_push(fixture, i); }
    RZ_TESTS_ASSERT_EQ(fixture->len, len);
    RZ_TESTS_ASSERT_FALSE(rz_arr_is_empty(fixture));

    {
        IntArrayView s = to_slice(fixture);
        rz_arr_strip_prefix_n(&s, 4u);
        helper_assert_arr_eq(s, {4, 5, 6, 7, 8, 9});
    }

    {
        IntArrayView s = to_slice(fixture);
        rz_arr_strip_suffix_n(&s, 4u);
        helper_assert_arr_eq(s, {0, 1, 2, 3, 4, 5});
    }
}
