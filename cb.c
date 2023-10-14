#define CB_IMPLEMENTATION
#include "cb.h"

cb_status_t on_configure(cb_t *cb, cb_config_t *cfg) {
    (void)cfg;
    cb_status_t  status       = CB_OK;
    cb_target_t *tests_target = cb_create_tests(cb, "tests_rz_array");
    status &= cb_target_add_sources_with_ext(tests_target, "./tests", "c", false);
    status &= cb_target_add_defines(tests_target, "UNIT_TEST", NULL);
    status &= cb_target_add_flags(tests_target, "-Wall", "-Wextra", "-pedantic", "-Wpedantic", "-ggdb", "-Os", NULL);
    status &= cb_target_add_includes(tests_target, "./include", NULL);
    cb_path_to_cstr(&tests_target->output);
    return status;
}

int main(int argc, char *argv[]) {
    CB_REBUILD_SELF(argc, argv);
    cb_t *cb = cb_init(argc, argv);
    if (!cb) return EXIT_FAILURE;
    if (cb_run(cb) == CB_ERR) return EXIT_FAILURE;
    cb_deinit(cb);
    return EXIT_SUCCESS;
}
