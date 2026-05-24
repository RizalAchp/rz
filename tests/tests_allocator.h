#pragma once

#ifndef __TEST_ALLOCATOR_H
#    define __TEST_ALLOCATOR

#    include "rz_allocator.h"
#    include "rz_collections.h"

RZ_DEF RZ_Allocator rz_test_allocator(RZ_Allocator base_allocator);

typedef struct {
    rz_uptr  ptr;
    rz_usize size;
    bool     freed;
    bool     double_free;
    bool     different_size_free;
    bool     different_size_remap;
    bool     remap_freed;
} RZ_Allocation;

typedef RZ_Array(RZ_Allocation) RZ_Allocations;
typedef struct {
    rz_usize total_allocated;
    rz_usize total_freed;
    rz_usize total_double_free;
    rz_usize total_different_size_free;
    rz_usize total_different_size_remap;
    rz_usize total_remap_freed;
} RZ_TallocResult;
typedef RZ_Opt(RZ_TallocResult) RZ_TallocResultOpt;

// typedef RZ_Opt(RZ_Allocations) RZ_TallocResult;
RZ_DEF RZ_TallocResultOpt rz_talloc_deinit(RZ_Allocator a);

RZ_DEF void               rz_talloc_deinit_without_check(RZ_Allocator a);
RZ_DEF RZ_TallocResultOpt rz_talloc_detect_leaks(RZ_Allocator a);

RZ_DEF const char *rz_talloc_format_result(RZ_Allocator a, RZ_TallocResult res);

#    define RZ_TESTS_ALLOCATOR_ASSERT_DEINIT(a)                            \
        RZ_TEMP_ALLOCATOR_BLOCK(ta, {                                      \
            RZ_TallocResultOpt res = rz_talloc_deinit(a);                  \
            if (res.is_some) {                                             \
                const char *msg = rz_talloc_format_result(ta, res.unwrap); \
                RZ_TESTS_ASSERT_FALSE(res.is_some, "%s", msg);             \
            }                                                              \
        })

#endif /* end of include guard: __TEST_ALLOCATOR_H */
