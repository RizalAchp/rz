#include "tests.h"

#define RZ_SET_IMPLEMENTATION
#include "../rz_set.h"

#define ARR_SIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

TEST_CASE(tests_rz_set_create_delete) {
    rz_oset_t oset = rz_oset_create(10);
    TEST_TRUE(rz_oset_is_empty(&oset));
    TEST_EQUAL(rz_oset_size(&oset), 0);
    TEST_EQUAL(oset.capacity, 10);

    rz_oset_delete(oset);
    TEST_TRUE(rz_oset_is_empty(&oset));
    TEST_EQUAL(rz_oset_size(&oset), 0);
    TEST_EQUAL(oset.items, NULL);
    TEST_EQUAL(oset.capacity, 0);
}

TEST_CASE(tests_rz_set_insert) {
    char const *result_case[] = {"hello", "world", "foo", "bar"};
    rz_oset_t   oset          = rz_oset_create(10);
    for (int i = 0; i < ARR_SIZE(result_case); ++i) rz_oset_insert_cstr(&oset, result_case[i]);
    TEST_EQUAL(rz_oset_size(&oset), 4);
    // insert hello again, the oset should not insert the string `hello` again, and the size is 4
    rz_oset_insert_cstr(&oset, "hello");
    TEST_EQUAL(rz_oset_size(&oset), 4);
    rz_oset_delete(oset);
    TEST_TRUE(rz_oset_is_empty(&oset));
    TEST_EQUAL(rz_oset_size(&oset), 0);
    TEST_EQUAL(oset.items, NULL);
    TEST_EQUAL(oset.capacity, 0);
}

TEST_CASE(tests_rz_set_contains) {
    char const *result_case[] = {"hello", "world", "foo", "bar"};
    rz_oset_t   oset          = rz_oset_create(10);
    for (int i = 0; i < ARR_SIZE(result_case); ++i) rz_oset_insert_cstr(&oset, result_case[i]);

    TEST_EQUAL(rz_oset_size(&oset), 4);
    TEST_FALSE(rz_oset_contains_cstr(&oset, "notcontains"));

    for (int i = 0; i < ARR_SIZE(result_case); ++i) TEST_TRUE(rz_oset_contains_cstr(&oset, result_case[i]));

    rz_oset_insert_cstr(&oset, "notcontains");
    TEST_EQUAL(rz_oset_size(&oset), 5);
    TEST_TRUE(rz_oset_contains_cstr(&oset, "notcontains"));

    rz_oset_delete(oset);
    TEST_TRUE(rz_oset_is_empty(&oset));
    TEST_EQUAL(rz_oset_size(&oset), 0);
    TEST_EQUAL(oset.items, NULL);
    TEST_EQUAL(oset.capacity, 0);
}
