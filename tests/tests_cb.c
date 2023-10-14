#define CB_IMPLEMENTATION
#include "../cb.h"
#include "tests.h"

cb_status_t on_configure(cb_t *cb, cb_config_t *cfg) {
    (void)cb, (void)cfg;
    return CB_OK;
}

TEST_CASE(test_cb_path_appends_operation) {
    cb_path_t p = cb_path("./build/cb");
    cb_path_append_cstr(&p, "hello");
    cb_path_append_cstr(&p, "world");
    TEST_TRUE((memcmp(p.data, "./build/cb/hello/world", p.count) == 0) && "cb_path_append_cstr");
}

TEST_CASE(test_cb_path_extension_operation) {
    cb_path_t p = cb_path("./build/cb.c");
    TEST_TRUE(cb_path_has_extension(&p) && "cb_path_has_extension `./build/cb.c`");
    cb_strview_t ext = cb_path_extension(&p);
    TEST_TRUE(cb_sv_eq(ext, cb_sv("c")) && "cb_path_extension `./build/cb.c`");
    TEST_TRUE(cb_path_with_extension(&p, "h") && "cb_path_with_extension `./build/cb.c`");
    TEST_TRUE(cb_sv_eq(cb_sv_from_parts(p.data, p.count), cb_sv("./build/cb.h")) && "cb_path_with_extension `./build/cb.c`");
}
