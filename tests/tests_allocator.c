#include "tests_allocator.h"
#include "rz_collections.h"
#include "rz_sprintf.h"
// #include "utest.h"

typedef struct {
    RZ_Allocations allocations;
    RZ_Allocator   base_allocator;
} RZ_TestAllocator;

static void *rz_test_alloc(void *o, rz_usize size);
static void *rz_test_remap(void *o, void *ptr, rz_usize ptrsize, rz_usize size);
static void  rz_test_dealloc(void *o, void *ptr, rz_usize ptrsize);

RZ_DEC RZ_Allocator rz_test_allocator(RZ_Allocator base_allocator) {
    static RZ_AllocatorVTable vtable   = {.alloc = rz_test_alloc, .dealloc = rz_test_dealloc, .remap = rz_test_remap};

    rz_usize          allocations_size = sizeof(RZ_TestAllocator);
    RZ_TestAllocator *test_allocator   = rz_raw_calloc(base_allocator, 1, allocations_size);
    RZ_ASSERT_ALLOCATOR_PTR(test_allocator);
    test_allocator->base_allocator        = base_allocator;
    test_allocator->allocations.allocator = base_allocator;
    rz_vec_append(&test_allocator->allocations, (RZ_Allocation){.ptr = (rz_uptr)test_allocator, .freed = false, .size = allocations_size});

    return (RZ_Allocator){.vtable = &vtable, .ptr = test_allocator};
}
RZ_DEC RZ_TallocResultOpt rz_talloc_deinit(RZ_Allocator a) {
    RZ_ASSERT_NOT_NULL(a.ptr);

    RZ_TallocResultOpt r = rz_talloc_detect_leaks(a);
    rz_talloc_deinit_without_check(a);
    return r;
}
RZ_DEC void rz_talloc_deinit_without_check(RZ_Allocator a) {
    if (a.ptr == NULL) return;
    RZ_TestAllocator *ta = a.ptr;
    rz_raw_free(ta->base_allocator, ta, sizeof(RZ_TestAllocator));
}

RZ_DEC RZ_TallocResultOpt rz_talloc_detect_leaks(RZ_Allocator a) {
    RZ_ASSERT_NOT_NULL(a.ptr);
    RZ_TestAllocator  *ta     = a.ptr;
    RZ_TallocResultOpt result = {0};

#define __check(name, rhs)               \
    if (it->name == rhs) {               \
        result.is_some = false;          \
        result.unwrap.total_##name += 1; \
    }

    // skip the first. because is used by allocating the allocator it self
    for (rz_usize i = 1; i < ta->allocations.len; ++i) {
        RZ_Allocation *it = &ta->allocations.data[i];
        result.unwrap.total_allocated += it->size;
        if (it->freed) {
            result.is_some = false;
            result.unwrap.total_freed += it->size;
        }
        __check(double_free, true);
        __check(different_size_free, true);
        __check(different_size_remap, true);
        __check(remap_freed, true);
    }
#undef __check

    return result;
}

// clang-format off
RZ_DEC const char *rz_talloc_format_result(RZ_TallocResult res) {
    const char *fres = rz_tsprintf(
        "Test Allocator Result:\n"
        "  total_allocated             = %zu B | %.3lf KB | %.2f MB\n"
        "  total_freed                 = %zu B | %.3lf KB | %.2f MB\n"
        "  total_double_free           = %zu allocations\n"
        "  total_different_size_free   = %zu allocations\n"
        "  total_different_size_remap  = %zu allocations\n"
        "  total_remap_freed           = %zu allocations\n",
        res.total_allocated, (rz_f64)res.total_allocated * 0.001, (rz_f64)res.total_allocated * 1e-6,
        res.total_freed, (rz_f64)res.total_freed * 0.001, (rz_f64)res.total_freed * 1e-6,
        res.total_double_free,
        res.total_different_size_free,
        res.total_different_size_remap,
        res.total_remap_freed
    );
    return fres;
}
// clang-format on

static void *rz_test_alloc(void *o, rz_usize size) {
    RZ_ASSERT(o != NULL && size != 0);
    RZ_TestAllocator *a = o;
    void             *p = rz_raw_alloc(a->base_allocator, size);
    if (p == NULL) return NULL;

    RZ_Allocation allocation = {.ptr = (rz_uptr)p, .size = size, .freed = false};
    rz_vec_append(&a->allocations, allocation);

    return p;
}

static void *rz_test_remap(void *o, void *ptr, rz_usize ptrsize, rz_usize size) {
    RZ_ASSERT(o != NULL && ptr != NULL && size != 0);
    RZ_TestAllocator *a       = o;

    RZ_Allocation *allocation = NULL;
    // find allocation if the ptr is not NULL (realloc new)
    rz_arr_foreach(it, &a->allocations) {
        if (it->ptr == (rz_uptr)ptr) { allocation = it; }
    }
    RZ_ASSERT(allocation, "allocation should be found in the rz_test_allocator: remap function");
    if (allocation->freed) { allocation->remap_freed = true; }
    if (allocation->size != ptrsize) { allocation->different_size_remap = true; }

    void *new_ptr = rz_raw_remap(a->base_allocator, ptr, ptrsize, size);
    if (new_ptr == NULL) { return NULL; }

    if ((rz_uptr)new_ptr != (rz_uptr)ptr) {
        // this means the os reallocate the location of ptr
        // set the allocation ptr into the new ptr
        allocation->ptr = (rz_uptr)new_ptr;
    }
    allocation->size = size;

    return new_ptr;
}

static void rz_test_dealloc(void *o, void *ptr, rz_usize ptrsize) {
    RZ_ASSERT(o != NULL && ptr != NULL, "guaranteed is not null. checked in the API layer");
    RZ_TestAllocator *a       = o;

    RZ_Allocation *allocation = NULL;
    // find allocation if the ptr is not NULL (realloc new)
    rz_arr_foreach(it, &a->allocations) {
        if (it->ptr == (rz_uptr)ptr) { allocation = it; }
    }
    RZ_ASSERT(allocation, "allocation should be found in the rz_test_allocator: dealloc function");

    if (allocation->freed) {
        // if ptr is already freed. set the double_free
        allocation->double_free = true;
    }
    if (allocation->size != ptrsize) {
        // if the size received in dealloc is different with metadata.
        // set the different_size_free
        allocation->different_size_free = true;
    }
    rz_raw_dealloc(a->base_allocator, ptr, ptrsize);
    allocation->freed = true;
}
