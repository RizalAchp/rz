#pragma once
#ifndef RZ_ALLOCATOR_H
#    define RZ_ALLOCATOR_H
#    include "rz_common.h"

/// Allocator System inspired by Zig Allocator's
#    if defined(__cplusplus)
extern "C" {
#    endif /* ifdef  defined(__cplus_plus) */

#    ifndef RZ_REALLOC
#        define RZ_REALLOC realloc
#    endif

#    ifndef RZ_FREE
#        define RZ_FREE free
#    endif

#    ifndef RZ_MALLOC_SIZE
#        if defined(RZ_OS_WINDOWS)
#            define RZ_MALLOC_SIZE _msize
#        elif defined(RZ_OS_APPLE)
#            define RZ_MALLOC_SIZE malloc_size
#        elif defined(RZ_OS_UNIX)
#            define RZ_MALLOC_SIZE malloc_usable_size
#        else
#        endif
#    endif

RZ_DEC rz_usize rz_mem_page_size(void);

// Allocator Interface
typedef struct RZ_Allocator RZ_Allocator;
// Allocator Interface VTable
typedef struct RZ_AllocatorVTable RZ_AllocatorVTable;

struct RZ_Allocator {
    void                     *ptr;
    const RZ_AllocatorVTable *vtable;
};

struct RZ_AllocatorVTable {
    void *(*alloc)(void *a, rz_usize len);
    void *(*remap)(void *a, void *mem, rz_usize mem_len, rz_usize new_len);
    void (*dealloc)(void *a, void *mem, rz_usize mem_len);
};

RZ_DEC void *rz_raw_alloc(RZ_Allocator a, rz_usize len);
RZ_DEC void *rz_raw_calloc(RZ_Allocator a, rz_usize num, rz_usize size);
RZ_DEC void *rz_raw_remap(RZ_Allocator a, void *mem, rz_usize mem_len, rz_usize new_len);
RZ_DEC void  rz_raw_dealloc(RZ_Allocator a, void *mem, rz_usize mem_len);

#    define rz_is_allocator(a)                   ((a).vtable != NULL)
#    define rz_raw_free                          rz_raw_dealloc

#    define rz_alloc_bytes(a, len)               rz_raw_alloc(a, len)
#    define rz_alloc(a, ptr, len)                rz_raw_alloc(a, sizeof(*(ptr)) * len)

#    define rz_calloc_bytes(a, len)              rz_raw_calloc(a, len, sizeof(rz_u8))
#    define rz_calloc(a, ptr, len)               rz_raw_calloc(a, len, sizeof(*(ptr)))

#    define rz_remap(a, ptr, ptr_len, new_len)   rz_raw_remap(a, (void *)ptr, (ptr_len) * sizeof(*(ptr)), (new_len) * sizeof(*(ptr)))
#    define rz_realloc(a, ptr, ptr_len, new_len) rz_remap(a, ptr, ptr_len, new_len)

#    define rz_dealloc(a, ptr, ptr_len)          rz_raw_dealloc(a, (void *)ptr, (ptr_len) * sizeof(*(ptr)))
#    define rz_free(a, ptr, ptr_len)             rz_raw_dealloc(a, (void *)ptr, (ptr_len) * sizeof(*(ptr)))

RZ_DEC void    *rz_memdup(RZ_Allocator a, const void *data, rz_usize size);
RZ_DEC rz_char *rz_strdup(RZ_Allocator a, const rz_char *cstr);
RZ_DEC void    *rz_memcat(RZ_Allocator a, const void *l, rz_usize ln, const void *r, rz_usize rn);

// Create Allocator for std_allocator (using Malloc, Realloc, Free, etc)
RZ_DEC RZ_Allocator rz_std_allocator(void);

#    ifndef RZ_ARENA_REGION_DEFAULT_CAPACITY
#        define RZ_ARENA_REGION_DEFAULT_CAPACITY (8 * 1024)
#    endif // RZ_ARENA_REGION_DEFAULT_CAPACITY

typedef struct RZ_ArenaAllocatorRegion RZ_ArenaAllocatorRegion;
typedef struct RZ_ArenaAllocator {
    RZ_Allocator             child_allocator;
    RZ_ArenaAllocatorRegion *begin, *end;
} RZ_ArenaAllocator;

typedef struct RZ_ArenaMark {
    RZ_ArenaAllocatorRegion *region;
    rz_usize                 count;
} RZ_ArenaMark;

RZ_DEC RZ_ArenaAllocator rz_arena(RZ_Allocator child_allocator);
RZ_DEC RZ_Allocator      rz_arena_allocator(RZ_ArenaAllocator *arena);

// destro the arena, free all memory and deinit

RZ_DEC RZ_ArenaMark rz_arena_snapshot(RZ_ArenaAllocator *a);
RZ_DEC void         rz_arena_reset(RZ_ArenaAllocator *a);
RZ_DEC void         rz_arena_rewind(RZ_ArenaAllocator *a, RZ_ArenaMark m);
RZ_DEC void         rz_arena_free(RZ_ArenaAllocator *a);
RZ_DEC void         rz_arena_trim(RZ_ArenaAllocator *a);

#    define RZ_ARENA_BLOCK(arena)                                          \
        RZ_ArenaMark m = rz_arena_snapshot(arena);                         \
        for (bool once = true; once; rz_arena_rewind(arena), once = false)

#    ifndef RZ_TEMP_ALLOCATOR_CAPACITY
#        define RZ_TEMP_ALLOCATOR_CAPACITY (8u * 1024u * 1024u)
#    endif

RZ_DEC RZ_Allocator rz_temp_allocator(void);
RZ_DEC RZ_ArenaMark rz_temp_snapshot(void);
RZ_DEC void         rz_temp_rewind(RZ_ArenaMark m);

#    define rz_tmemdup(data, size)   rz_memdup(rz_temp_allocator(), data, size)
#    define rz_tstrdup(cstr)         rz_strdup(rz_temp_allocator(), cstr)
#    define rz_tmemcat(l, ln, r, rn) rz_memcat(rz_temp_allocator(), l, ln, r, rn)

#    define RZ_TEMP_ALLOCATOR_BLOCK(allocator, ...)                     \
        do {                                                            \
            RZ_ArenaMark __temp_allocator_mark__ = rz_temp_snapshot();  \
            RZ_Allocator allocator               = rz_temp_allocator(); \
            do {                                                        \
                __VA_ARGS__                                             \
            } while (0);                                                \
            rz_temp_rewind(__temp_allocator_mark__);                    \
        } while (0)

// RZ_DEC void       *rz_memcpy(void *restrict dest, const void *restrict src, rz_usize src_n);
// RZ_DEC void       *rz_memmove(void *dest, const void *src, size_t src_n);
// RZ_DEC const void *rz_memchr(const void *src, rz_int c, size_t n);
// RZ_DEC rz_ptrdiff  rz_memcmp(const void *lhs, const void *rhs, rz_usize size);
// RZ_DEC void       *rz_memset(void *dest, rz_i32 c, rz_usize n);

#    define rz_memcpy  memcpy
#    define rz_memmove memmove
#    define rz_memchr  memchr
#    define rz_memcmp  memcmp
#    define rz_memset  memset

RZ_DEC bool rz_memequal(const void *lhs, rz_usize lhs_size, const void *rhs, rz_usize rhs_size);
RZ_DEC bool rz_memswap(void *lhs, void *rhs, rz_usize n);
#    define rz_memzero(dest, n) rz_memset(dest, 0, n)

// RZ_DEC

#    if defined(__cplusplus)
}
#    endif /* ifdef  defined(__cplus_plus) */

#endif     /* end of include guard: RZ_ALLOCATOR_H */
