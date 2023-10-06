#include "tests.h"

#define RZ_HASH_IMPLEMENTATION
#include "../rz_hash.h"

#define ARR_SIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

const char *sample_keys[] = {"Hello, World!",
                             "This is a sample string.",
                             "C programming is fun.",
                             "Arrays in C are versatile.",
                             "Coding is a valuable skill.",
                             "Keep learning and coding.",
                             "Programming languages vary.",
                             "Practice makes perfect.",
                             "Computer science is fascinating.",
                             "Algorithms are important.",
                             "Data structures matter.",
                             "Pointers can be tricky.",
                             "Memory management is crucial.",
                             "Debugging is part of coding.",
                             "Software development is creative.",
                             "C is used for system programming.",
                             "Linux is open source.",
                             "Windows is a popular OS.",
                             "MacOS has a user-friendly interface.",
                             "Version control is important.",
                             "Git is widely used.",
                             "GitHub is a popular platform.",
                             "Coding interviews test skills.",
                             "Problem-solving is key.",
                             "Coding bootcamps can help.",
                             "Web development is in demand.",
                             "JavaScript is a web language.",
                             "HTML and CSS are fundamental.",
                             "Python is known for simplicity.",
                             "Java is used in enterprise.",
                             "Machine learning is exciting.",
                             "AI is transforming industries.",
                             "Cybersecurity is critical.",
                             "The cloud is everywhere.",
                             "Mobile app development is hot.",
                             "React Native is for mobile.",
                             "Databases store data.",
                             "SQL is a database language.",
                             "NoSQL databases are flexible.",
                             "APIs enable software integration.",
                             "DevOps bridges development and IT.",
                             "Continuous integration is efficient.",
                             "Agile methodologies promote collaboration.",
                             "Software testing ensures quality.",
                             "User experience matters.",
                             "UI design impacts usability.",
                             "Accessibility is essential.",
                             "Responsive web design adapts.",
                             "Tech careers offer opportunities.",
                             "Programming is a lifelong journey."};

const char *to_remove[]   = {
    "C programming is fun.",
    "Data structures matter.",
    "Cybersecurity is critical.",
};

TEST_CASE(tests_rz_hset_create_delete) {
    rz_hset_t set = rz_hset_create(100);
    TEST_TRUE(rz_hset_is_empty(&set));
    TEST_EQUAL(rz_hset_size(&set), 0);
    TEST_EQUAL(set.capacity, 100);

    rz_hset_delete(&set);
    TEST_TRUE(rz_hset_is_empty(&set));
    TEST_EQUAL(rz_hset_size(&set), 0);
    TEST_EQUAL(set.table, NULL);
    TEST_EQUAL(set.capacity, 0);
}

TEST_CASE(tests_rz_hset_insert) {
    rz_hset_t set = rz_hset_create(10);
    for (size_t i = 0; i < ARR_SIZE(sample_keys); ++i) rz_hset_insert_cstr(&set, sample_keys[i]);
    TEST_EQUAL(rz_hset_size(&set), ARR_SIZE(sample_keys));
    // insert hello again, the set should not insert the string of first sample_keys again, and the size is remain
    TEST_EQUAL(rz_hset_insert_cstr(&set, sample_keys[0]), RZ_HASH_COLLISION_ERR);
    TEST_EQUAL(rz_hset_size(&set), ARR_SIZE(sample_keys));
    rz_hset_delete(&set);
}

TEST_CASE(tests_rz_hset_contains) {
    rz_hset_t set = rz_hset_create(10);
    for (size_t i = 0; i < ARR_SIZE(sample_keys); ++i) rz_hset_insert_cstr(&set, sample_keys[i]);

    TEST_EQUAL(rz_hset_size(&set), ARR_SIZE(sample_keys));
    TEST_EQUAL(rz_hset_contains_cstr(&set, "notcontains"), RZ_HASH_ERR);

    for (size_t i = 0; i < ARR_SIZE(sample_keys); ++i) TEST_EQUAL(rz_hset_contains_cstr(&set, sample_keys[i]), RZ_HASH_OK);

    TEST_EQUAL(rz_hset_insert_cstr(&set, "notcontains"), RZ_HASH_OK);
    TEST_EQUAL(rz_hset_size(&set), ARR_SIZE(sample_keys) + 1);
    TEST_EQUAL(rz_hset_contains_cstr(&set, "notcontains"), RZ_HASH_OK);
    rz_hset_delete(&set);
}

TEST_CASE(tests_rz_hset_remove) {
    rz_hset_t set = rz_hset_create(10);
    for (size_t i = 0; i < ARR_SIZE(sample_keys); ++i) TEST_EQUAL(rz_hset_insert_cstr(&set, sample_keys[i]), RZ_HASH_OK);
    TEST_EQUAL(rz_hset_size(&set), ARR_SIZE(sample_keys));
    TEST_EQUAL(rz_hset_remove_cstr(&set, "try to remove non existense string"), RZ_HASH_NOT_FOUND_ERR);

    for (size_t i = 0; i < ARR_SIZE(to_remove); ++i) TEST_EQUAL(rz_hset_remove_cstr(&set, to_remove[i]), RZ_HASH_OK);
    TEST_EQUAL(rz_hset_size(&set), (ARR_SIZE(sample_keys) - ARR_SIZE(to_remove)));

    for (size_t i = 0; i < ARR_SIZE(to_remove); ++i) TEST_EQUAL(rz_hset_contains_cstr(&set, to_remove[i]), RZ_HASH_ERR);
    rz_hset_delete(&set);
}

TEST_CASE(tests_rz_hmap_create_delete) {
    rz_hmap_t map = rz_hmap_create(100);
    TEST_TRUE(rz_hmap_is_empty(&map));
    TEST_EQUAL(rz_hmap_size(&map), 0);
    TEST_EQUAL(map.capacity, 100);

    rz_hmap_delete(&map);
    TEST_TRUE(rz_hmap_is_empty(&map));
    TEST_EQUAL(rz_hmap_size(&map), 0);
    TEST_EQUAL(map.table, NULL);
    TEST_EQUAL(map.capacity, 0);
}

TEST_CASE(tests_rz_hmap_insert) {
    rz_hmap_t map = rz_hmap_create(10);
    for (size_t i = 0; i < ARR_SIZE(sample_keys); ++i) {
        const char *key = sample_keys[i];
        rz_hmap_insert(&map, key, "value", sizeof("value"));
    }
    TEST_EQUAL(rz_hmap_size(&map), ARR_SIZE(sample_keys));
    // insert hello again, the map should not insert the string of first sample_keys again, and the size is remain
    TEST_EQUAL(rz_hmap_insert(&map, sample_keys[0], "value", sizeof("value")), RZ_HASH_COLLISION_ERR);
    TEST_EQUAL(rz_hmap_size(&map), ARR_SIZE(sample_keys));
    rz_hmap_delete(&map);
}

TEST_CASE(tests_rz_hmap_contains) {
    rz_hmap_t map = rz_hmap_create(10);
    for (size_t i = 0; i < ARR_SIZE(sample_keys); ++i) rz_hmap_insert(&map, sample_keys[i], "value", sizeof("value"));

    TEST_EQUAL(rz_hmap_size(&map), ARR_SIZE(sample_keys));
    TEST_EQUAL(rz_hmap_contains(&map, "notcontains"), RZ_HASH_ERR);

    for (size_t i = 0; i < ARR_SIZE(sample_keys); ++i) TEST_EQUAL(rz_hmap_contains(&map, sample_keys[i]), RZ_HASH_OK);

    TEST_EQUAL(rz_hmap_insert(&map, "notcontains", "value", sizeof("value")), RZ_HASH_OK);
    TEST_EQUAL(rz_hmap_size(&map), ARR_SIZE(sample_keys) + 1);
    TEST_EQUAL(rz_hmap_contains(&map, "notcontains"), RZ_HASH_OK);
    rz_hmap_delete(&map);
}

TEST_CASE(tests_rz_hmap_remove) {
    rz_hmap_t map = rz_hmap_create(10);
    for (size_t i = 0; i < ARR_SIZE(sample_keys); ++i) TEST_EQUAL(rz_hmap_insert(&map, sample_keys[i], "value", sizeof("value")), RZ_HASH_OK);
    TEST_EQUAL(rz_hmap_size(&map), ARR_SIZE(sample_keys));
    TEST_EQUAL(rz_hmap_remove(&map, "try to remove non existense string"), RZ_HASH_NOT_FOUND_ERR);

    for (size_t i = 0; i < ARR_SIZE(to_remove); ++i) TEST_EQUAL(rz_hmap_remove(&map, to_remove[i]), RZ_HASH_OK);
    TEST_EQUAL(rz_hmap_size(&map), (ARR_SIZE(sample_keys) - ARR_SIZE(to_remove)));

    for (size_t i = 0; i < ARR_SIZE(to_remove); ++i) TEST_EQUAL(rz_hmap_contains(&map, to_remove[i]), RZ_HASH_ERR);
    rz_hmap_delete(&map);
}
