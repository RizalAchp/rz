#define RZ_TESTS_IMPL
#include "rz_common.h"
#include "rz_tests.h"

#include "rz_collections.h"
#include "tests_allocator.h"

RZ_TESTS_MAIN()

typedef RZ_Hm(const char *, rz_int) HashMapStr;
typedef RZ_Hs(const char *) HashSetStr;

typedef struct {
    rz_usize country_code;
    rz_usize poscode;
    rz_usize nip;
} TestCustomKey;
typedef RZ_Hm(TestCustomKey, rz_int) HashMapCustomKey;
typedef RZ_Hs(TestCustomKey) HashSetCustomKey;

#define assert_hm_empty(hm)              \
    RZ_TESTS_ASSERT_EQ((hm)->len, 0u);   \
    RZ_TESTS_ASSERT_EQ((hm)->data, NULL)

typedef struct {
    RZ_Allocator     alc;
    HashMapStr       map_str;
    HashSetStr       set_str;
    HashMapCustomKey map_custom;
    HashSetCustomKey set_custom;
} Fixture;
static rz_usize custom_key_hashcmp(RZ_HmHashCmpOp op, void const *a, void const *b, rz_usize len, rz_usize seed) {
    RZ_UNUSED(len);
    switch (op) {
    case RZ_HM_HASHCMP_HASH:
        const TestCustomKey *aa = a;
        const TestCustomKey *bb = b;
        return (aa->nip == bb->nip) && (aa->poscode == bb->poscode);

        break;
    case RZ_HM_HASHCMP_CMP:
        return rz_hm_hasheq_bytes(op, a, b, sizeof(TestCustomKey), seed);
        break;
    default:
        RZ_UNREACHABLE("custom_key_hashcmp: RZ_HmHashCmpOp");
    }
}

RZ_TESTS_SETUP(Fixture) {
    auto a       = rz_test_allocator(rz_std_allocator());
    fixture->alc = a;
    assert_hm_empty(&fixture->map_str);
    assert_hm_empty(&fixture->set_str);
    assert_hm_empty(&fixture->map_custom);
    assert_hm_empty(&fixture->set_custom);

    rz_hm_init(&fixture->map_str, .allocator = a, .hashcmp = rz_hm_hasheq_string);
    rz_hm_init(&fixture->set_str, .allocator = a, .hashcmp = rz_hm_hasheq_string);
    rz_hm_init(&fixture->map_custom, .allocator = a, .hashcmp = custom_key_hashcmp);
    rz_hm_init(&fixture->set_custom, .allocator = a, .hashcmp = custom_key_hashcmp);
}

RZ_TESTS_TEARDOWN(Fixture) {
    rz_hm_free(&fixture->map_str);
    rz_hm_free(&fixture->set_str);
    rz_hm_free(&fixture->map_custom);
    rz_hm_free(&fixture->set_custom);

    assert_hm_empty(&fixture->map_str);
    assert_hm_empty(&fixture->set_str);
    assert_hm_empty(&fixture->map_custom);
    assert_hm_empty(&fixture->set_custom);

    RZ_TESTS_ALLOCATOR_ASSERT_DEINIT(fixture->alc);
}

RZ_TESTS(Fixture, s) {
    RZ_UNUSED(ctx);
    RZ_UNUSED(fixture);
}
