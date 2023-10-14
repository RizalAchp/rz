#include "../rz_array.h"
#include "tests.h"

#define ARRAY_GENERIC_TESTS_CASE(type)                                                             \
    TEST_CASE(tests_rz_array_##type##_create_delete) {                                             \
        rz_arr_##type##_t arr = rz_arr_##type##_create(10);                                        \
        TEST_EQUAL(rz_arr_size(&arr), 0);                                                          \
        TEST_EQUAL(arr.capacity, 10);                                                              \
        rz_arr_##type##_delete(&arr);                                                              \
        TEST_EQUAL(rz_arr_size(&arr), 0);                                                          \
        TEST_EQUAL(arr.items, NULL);                                                               \
        TEST_EQUAL(arr.count, 0);                                                                  \
        TEST_EQUAL(arr.capacity, 0);                                                               \
    }                                                                                              \
    TEST_CASE(tests_rz_array_##type##_push_front_back) {                                           \
        rz_arr_##type##_t arr = rz_arr_##type##_create(10);                                        \
        TEST_EQUAL(rz_arr_size(&arr), 0);                                                          \
        TEST_EQUAL(arr.capacity, 10);                                                              \
                                                                                                   \
        rz_arr_##type##_push_back(&arr, (type)69);                                                 \
        TEST_EQUAL(arr.items[0], (type)69);                                                        \
        rz_arr_##type##_push_back(&arr, (type)212);                                                \
        TEST_EQUAL(arr.items[1], (type)212);                                                       \
        TEST_EQUAL(rz_arr_size(&arr), (type)2);                                                    \
                                                                                                   \
        rz_arr_##type##_push_front(&arr, (type)33);                                                \
        TEST_EQUAL(arr.items[0], (type)33);                                                        \
        rz_arr_##type##_push_front(&arr, (type)64);                                                \
        TEST_EQUAL(arr.items[0], (type)64);                                                        \
        TEST_EQUAL(arr.items[1], (type)33);                                                        \
        TEST_EQUAL(rz_arr_size(&arr), (type)4);                                                    \
                                                                                                   \
        TEST_EQUAL(arr.items[2], (type)69);                                                        \
        TEST_EQUAL(arr.items[3], (type)212);                                                       \
                                                                                                   \
        rz_arr_##type##_delete(&arr);                                                              \
        TEST_EQUAL(rz_arr_size(&arr), 0);                                                          \
        TEST_EQUAL(arr.items, NULL);                                                               \
        TEST_EQUAL(arr.count, 0);                                                                  \
        TEST_EQUAL(arr.capacity, 0);                                                               \
    }                                                                                              \
                                                                                                   \
    TEST_CASE(tests_rz_array_##type##_remove) {                                                    \
        type              expected_arr[] = {(type)1, (type)2, (type)4, (type)5, (type)7, (type)8}; \
        rz_arr_##type##_t arr            = rz_arr_##type##_create(10);                             \
        TEST_EQUAL(rz_arr_size(&arr), 0);                                                          \
        TEST_EQUAL(arr.capacity, 10);                                                              \
                                                                                                   \
        for (type idx = 0; idx < 10; idx++) {                                                      \
            rz_arr_##type##_push_back(&arr, (type)idx);                                            \
        }                                                                                          \
        TEST_EQUAL(rz_arr_size(&arr), (type)10);                                                   \
        TEST_EQUAL(rz_arr_##type##_remove(&arr, (type)0), RZ_ARRAY_OK);                            \
        TEST_EQUAL(rz_arr_##type##_remove(&arr, (type)3), RZ_ARRAY_OK);                            \
        TEST_EQUAL(rz_arr_##type##_remove(&arr, (type)6), RZ_ARRAY_OK);                            \
        TEST_EQUAL(rz_arr_##type##_remove(&arr, (type)9), RZ_ARRAY_OK);                            \
        TEST_EQUAL(rz_arr_size(&arr), (type)6);                                                    \
        type *items     = arr.items;                                                               \
        type *expecteds = expected_arr;                                                            \
        for (type idx = 0; (size_t)idx < (sizeof(expected_arr) / sizeof(*expected_arr)); idx++) {  \
            TEST_EQUAL(*(items++), *(expecteds++));                                                \
        }                                                                                          \
        rz_arr_##type##_delete(&arr);                                                              \
        TEST_EQUAL(rz_arr_size(&arr), 0);                                                          \
        TEST_EQUAL(arr.items, NULL);                                                               \
        TEST_EQUAL(arr.count, 0);                                                                  \
        TEST_EQUAL(arr.capacity, 0);                                                               \
    }

ARRAY_GENERIC_TESTS_CASE(s8)
ARRAY_GENERIC_TESTS_CASE(s16)
ARRAY_GENERIC_TESTS_CASE(s32)
ARRAY_GENERIC_TESTS_CASE(s64)
ARRAY_GENERIC_TESTS_CASE(u8)
ARRAY_GENERIC_TESTS_CASE(u16)
ARRAY_GENERIC_TESTS_CASE(u32)
ARRAY_GENERIC_TESTS_CASE(u64)
ARRAY_GENERIC_TESTS_CASE(f32)
ARRAY_GENERIC_TESTS_CASE(f64)
ARRAY_GENERIC_TESTS_CASE(rz_usize)
