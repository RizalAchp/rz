#include "rz_allocator.h"

#if defined(RZ_OS_APPLE)
#    if RZ_ARCH_TAG == RZ_ARCH_X86_64
#        define RZ_PAGE_SIZE_MIN 4 << 10
#        define RZ_PAGE_SIZE_MAX 4 << 10
#    elif RZ_ARCH_TAG == RZ_ARCH_AARCH64
#        define RZ_PAGE_SIZE_MIN 16 << 10
#        define RZ_PAGE_SIZE_MAX 16 << 10
#    else
#        define RZ_PAGE_SIZE_MIN 1
#        define RZ_PAGE_SIZE_MAX 1
#    endif
#elif defined(RZ_OS_WINDOWS)
#    define RZ_PAGE_SIZE_MIN 4 << 10
#    define RZ_PAGE_SIZE_MAX 4 << 10
#elif defined(RZ_OS_FREEBSD)
#    define RZ_PAGE_SIZE_MIN 4 << 10
#    define RZ_PAGE_SIZE_MAX 4 << 10
#elif defined(RZ_OS_NETBSD)
#    define RZ_PAGE_SIZE_MIN 4 << 10
#    if RZ_ARCH_TAG == RZ_ARCH_X86_64 || RZ_ARCH_TAG == RZ_ARCH_X86 || RZ_ARCH_TAG == RZ_ARCH_ARM || RZ_ARCH_TAG == RZ_ARCH_RISCV32 || RZ_ARCH_TAG == RZ_ARCH_RISCV64
#        define RZ_PAGE_SIZE_MAX 4 << 10
#    elif RZ_ARCH_TAG == RZ_ARCH_AARCH64
#        define RZ_PAGE_SIZE_MAX 64 << 10
#    elif RZ_ARCH_TAG == RZ_ARCH_MIPS || RZ_ARCH_TAG == RZ_ARCH_MIPS64 || RZ_ARCH_TAG == RZ_ARCH_POWERPC || RZ_ARCH_TAG == RZ_ARCH_POWERPC64
#        define RZ_PAGE_SIZE_MAX 16 << 10
#    else
#        define RZ_PAGE_SIZE_MIN 1
#        define RZ_PAGE_SIZE_MAX 1
#    endif
#elif defined(RZ_OS_OPENBSD)
#    define RZ_PAGE_SIZE_MIN 4 << 10
#    if RZ_ARCH_TAG == RZ_ARCH_X86_64 || RZ_ARCH_TAG == RZ_ARCH_X86 || RZ_ARCH_TAG == RZ_ARCH_ARM || RZ_ARCH_TAG == RZ_ARCH_RISCV32 || RZ_ARCH_TAG == RZ_ARCH_RISCV64 || \
        RZ_ARCH_TAG == RZ_ARCH_AARCH64 || RZ_ARCH_TAG == RZ_ARCH_POWERPC || RZ_ARCH_TAG == RZ_ARCH_POWERPC64
#        define RZ_PAGE_SIZE_MAX 4 << 10
#    elif RZ_ARCH_TAG == RZ_ARCH_MIPS || RZ_ARCH_TAG == RZ_ARCH_MIPS64
#        define RZ_PAGE_SIZE_MAX 16 << 10
#    else
#        define RZ_PAGE_SIZE_MIN 1
#        define RZ_PAGE_SIZE_MAX 1
#    endif
#elif defined(RZ_OS_HAIKU)
#    define RZ_PAGE_SIZE_MIN 4 << 10
#    define RZ_PAGE_SIZE_MAX 4 << 10
#elif defined(RZ_OS_LINUX)
#    define RZ_PAGE_SIZE_MIN 4 << 10
#    if RZ_ARCH_TAG == RZ_ARCH_X86_64 || RZ_ARCH_TAG == RZ_ARCH_X86 || RZ_ARCH_TAG == RZ_ARCH_ARM || RZ_ARCH_TAG == RZ_ARCH_RISCV32 || RZ_ARCH_TAG == RZ_ARCH_RISCV64 || \
        RZ_ARCH_TAG == RZ_ARCH_XTENSA
#        define RZ_PAGE_SIZE_MAX 4 << 10
#    elif RZ_ARCH_TAG == RZ_ARCH_AARCH64 || RZ_ARCH_TAG == RZ_ARCH_MIPS || RZ_ARCH_TAG == RZ_ARCH_MIPS64
#        define RZ_PAGE_SIZE_MAX 64 << 10
#    elif RZ_ARCH_TAG == RZ_ARCH_POWERPC || RZ_ARCH_TAG == RZ_ARCH_POWERPC64
#        define RZ_PAGE_SIZE_MAX 256 << 10
#    else
#        define RZ_PAGE_SIZE_MIN 1
#        define RZ_PAGE_SIZE_MAX 1
#    endif
#elif defined(RZ_OS_EMSCRIPTEN) || defined(RZ_OS_WASI)
#    define RZ_PAGE_SIZE_MIN 64 << 10
#    define RZ_PAGE_SIZE_MAX 64 << 10
#else
#    if (RZ_ARCH_TAG == RZ_ARCH_WASM32 || RZ_ARCH_TAG == RZ_ARCH_WASM64)
#        define RZ_PAGE_SIZE_MIN 64 << 10
#        define RZ_PAGE_SIZE_MAX 64 << 10
#    elif (RZ_ARCH_TAG == RZ_ARCH_X86_64 || RZ_ARCH_TAG == RZ_ARCH_X86) || (RZ_ARCH_TAG == RZ_ARCH_AARCH64)
#        define RZ_PAGE_SIZE_MIN 4 << 10
#        define RZ_PAGE_SIZE_MAX 4 << 10
#    else
#        error "Unknown OS or Arch"
#    endif
#endif

#ifdef RZ_ALLOC_IMPL

RZ_DEF void *rz_raw_alloc(RZ_Allocator a, rz_usize len) {
    RZ_ASSERT_NOT_NULL(a.vtable);
    RZ_ASSERT_NOT_NULL(a.vtable->alloc);
    RZ_ASSERT(len == 0 && "trying to allocate memory with 0 size");

    return a.vtable->alloc(a.ptr, len);
}

RZ_DEF void *rz_raw_remap(RZ_Allocator a, void *mem, rz_usize mem_len, rz_usize new_len) {
    RZ_ASSERT_NOT_NULL(a.vtable);
    RZ_ASSERT_NOT_NULL(a.vtable->remap);
    // if try to remap memory with smaller size, just return.
    if (new_len <= mem_len) return mem;

    /// forward to alloc new
    if (mem == NULL || mem_len == 0) return rz_raw_alloc(a, new_len);
    return a.vtable->remap(a.ptr, mem, mem_len, new_len);
}

RZ_DEF void rz_raw_dealloc(RZ_Allocator a, void *mem, rz_usize mem_len) {
    RZ_ASSERT_NOT_NULL(a.vtable);
    RZ_ASSERT_NOT_NULL(a.vtable->dealloc);

    if (mem == NULL || mem_len == 0) return;
    a.vtable->dealloc(a.ptr, mem, mem_len);
}

RZ_DEF void *rz_raw_calloc(RZ_Allocator a, rz_usize num, rz_usize size) {
    rz_usize bytes_size = num * size;
    void    *ptr        = rz_raw_alloc(a, bytes_size);
    if (ptr == NULL) return ptr;

    return rz_memset(ptr, 0, bytes_size);
}

RZ_DEF void *rz_memdup(RZ_Allocator a, const void *data, rz_usize size) {
    void *new_data = rz_raw_alloc(a, size);
    if (new_data == NULL) return NULL;
    rz_memcpy(new_data, data, size);
    return new_data;
}

RZ_DEF rz_char *rz_strdup(RZ_Allocator a, const rz_char *str) {
    rz_usize size = strlen(str);
    return rz_memdup(a, str, size);
}

RZ_DEF void *rz_memcat(RZ_Allocator a, const void *l, rz_usize ln, const void *r, rz_usize rn) {
    rz_u8 *res = rz_alloc_bytes(a, ln + rn);
    if (res == NULL) return NULL;
    rz_memcpy(res, l, ln);
    rz_memcpy(res + ln, r, rn);
    return res;
}

static RZ_DEF rz_usize rz__query_page_size(void) {
    static rz_usize size = 0;
    if (size > 0) return size;

#    if defined(RZ_OS_WINDOWS)
    /* Windows */
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    size = si.dwPageSize;
#    elif defined(RZ_OS_LINUX)

#        if (defined(__GNU_LIBRARY__) || defined(__GLIBC__) || defined(__GLIBC_MINOR__)) || defined(_SC_PAGE_SIZE)
    /* POSIX systems (Linux, macOS, BSD, etc.) */
    long page_size = sysconf(_SC_PAGE_SIZE);
    size           = (page_size > 0) ? (rz_usize)page_size : 4096;
#        else
    size = getauxval(AT_PAGESZ);
#        endif
#    elif defined(RZ_OS_APPLE)
    task_t task_port = mach_task_self();
    if (task_port == MACH_PORT_NULL) return 0;

    mach_msg_type_number_t info_count = TASK_VM_INFO_COUNT;
    // Initialize to zero for safety
    task_vm_info_data_t vm_info = {.page_size = 0};
    kern_return_t       kr      = task_info(task_port, TASK_VM_INFO, (task_info_t)&vm_info, &info_count);
    if (kr != KERN_SUCCESS) return 0;
    size = vm_info.page_size;
#    else
#        if (defined(__GNU_LIBRARY__) || defined(__GLIBC__) || defined(__GLIBC_MINOR__)) || defined(_SC_PAGE_SIZE)
    /* POSIX systems (Linux, macOS, BSD, etc.) */
    long page_size = sysconf(_SC_PAGE_SIZE);
    size           = (page_size > 0) ? (rz_usize)page_size : 4096;
#        else
#            error "page size not supported without linking libc, using the default implementation"
#        endif
#    endif
    if (size == 0) size = RZ_PAGE_SIZE_MAX;
    RZ_ASSERT((size >= RZ_PAGE_SIZE_MIN) && (size <= RZ_PAGE_SIZE_MAX));
    return size;
}

RZ_DEF rz_usize rz_mem_page_size(void) {
    return ((RZ_PAGE_SIZE_MIN == RZ_PAGE_SIZE_MAX) ? RZ_PAGE_SIZE_MIN : rz__query_page_size());
}

#endif // RZ_ALLOC_IMPL

#ifdef RZ_ALLOC_STD_IMPL

// return opaque pointer allocated, or NULL if error.
static void *rz__astd_alloc(void *a, rz_usize len);
static void *rz__astd_remap(void *a, void *mem, rz_usize mem_len, rz_usize new_len);
static void  rz__astd_dealloc(void *a, void *mem, rz_usize mem_len);
static bool  rz__astd_mem_size(void *a, void *mem, rz_usize mem_len, rz_usize new_len);

RZ_DEF RZ_Allocator rz_std_allocator(void) {
    static const RZ_AllocatorVTable rz__global_allocator_vtable = {
        .alloc   = rz__astd_alloc,
        .remap   = rz__astd_remap,
        .dealloc = rz__astd_dealloc,
    };

    RZ_Allocator a = {0};
    a.vtable       = &rz__global_allocator_vtable;
    return a;
}

void *rz__astd_alloc(void *opaque, rz_usize ptr_size) {
    RZ_UNUSED(opaque);
    RZ_ASSERT(ptr_size > 0, "Try to allocate 0 (zero) len");

    void *ptr = RZ_REALLOC(NULL, RZ_MAX(ptr_size, alignof(max_align_t)));
    if (ptr == NULL) return NULL;
    return ptr;
}

bool rz__astd_mem_size(void *a, void *ptr, rz_usize ptr_len, rz_usize new_len) {
    RZ_UNUSED(a);
    RZ_ASSERT(ptr != NULL && ptr_len != 0, "ptr should not NULL in this point");
    RZ_ASSERT(new_len > 0, "Try to re-allocate/resize to size 0 (zero) len");
    if (new_len <= ptr_len) {
        // in-place shrink always works
        return ptr_len;
    }
#    ifdef RZ_MALLOC_SIZE
    return RZ_MALLOC_SIZE(ptr);
#    endif
    return ptr_len;
}

void *rz__astd_remap(void *opaque, void *ptr, rz_usize ptr_len, rz_usize new_len) {
    RZ_UNUSED(opaque);
    RZ_ASSERT(ptr != NULL && ptr_len != 0, "ptr should not NULL in this point");
    RZ_ASSERT(new_len > 0, "Try to re-allocate/resize to size 0 (zero) len");

    // Prefer resizing in-place if possible, since `realloc` could be expensive
    // even if legal.
    ptr_len = rz__astd_mem_size(opaque, ptr, ptr_len, new_len);
    if (new_len <= ptr_len) return ptr;

    // `malloc` and friends guarantee the required alignment, so we can try
    // `realloc`. C only needs to respect `max_t` up to the allocation
    // size due to object alignment rules. If necessary, extend the allocation
    // size.
    rz_usize actual_len = RZ_MAX(new_len, alignof(max_align_t));
    void    *new_ptr    = RZ_REALLOC(ptr, actual_len);
    if (new_ptr == NULL) return NULL;
    return new_ptr;
}

void rz__astd_dealloc(void *a, void *mem, rz_usize mem_len) {
    RZ_UNUSED(a);
    RZ_UNUSED(mem_len);
    RZ_FREE(mem);
}

#endif /* ifdef RZ_ALLOC_STD_IMPL */

#ifdef RZ_ALLOC_PAGE_IMPL

#endif /* ifdef RZ_ALLOC_PAGE_IMPL */

#ifdef RZ_ALLOC_ARENA_IMPL
struct RZ_ArenaAllocatorRegion {
    RZ_ArenaAllocatorRegion *next;
    rz_usize                 count;
    rz_usize                 capacity;
    rz_uptr                  data[];
};

RZ_ArenaAllocatorRegion *rz__new_region(rz_usize capacity, RZ_Allocator *a) {
    rz_usize                 size_bytes = sizeof(RZ_ArenaAllocatorRegion) + sizeof(rz_uptr) * capacity;
    RZ_ArenaAllocatorRegion *r          = rz_raw_alloc(*a, size_bytes);
    if (r == NULL) return NULL;
    r->next     = NULL;
    r->count    = 0;
    r->capacity = capacity;
    return r;
}

#    define rz__free_region(r, a) rz_raw_free(a, r, sizeof(RZ_ArenaAllocatorRegion) + sizeof(rz_uptr) * (r)->capacity)

RZ_DEF RZ_ArenaAllocator rz_arena(RZ_Allocator child_allocator) {
    RZ_ArenaAllocator arena = {0};
    arena.child_allocator   = child_allocator;
    return arena;
}
static void              *rz__arena_alloc(void *opaque, rz_usize size_bytes);
static void              *rz__arena_remap(void *a, void *mem, rz_usize mem_len, rz_usize new_len);
static void               rz__arena_dealloc(void *a, void *mem, rz_usize mem_len);
static RZ_AllocatorVTable rz__arena_allocator_vtable = {
    .alloc   = rz__arena_alloc,
    .remap   = rz__arena_remap,
    .dealloc = rz__arena_dealloc,
};

void *rz__arena_alloc(void *opaque, rz_usize size_bytes) {
    RZ_DBG_ASSERT_NOT_NULL(opaque);
    RZ_ArenaAllocator *a    = (RZ_ArenaAllocator *)opaque;
    rz_usize           size = (size_bytes + sizeof(rz_uptr) - 1) / sizeof(rz_uptr);

    if (a->end == NULL) {
        RZ_ASSERT(a->begin == NULL);
        rz_usize capacity = RZ_ARENA_REGION_DEFAULT_CAPACITY;
        if (capacity < size) capacity = size;
        a->end   = rz__new_region(capacity, &a->child_allocator);
        a->begin = a->end;
    }

    while ((a->end->count + size) > a->end->capacity && a->end->next != NULL) {
        a->end = a->end->next;
    }

    if ((a->end->count + size) > a->end->capacity) {
        RZ_ASSERT(a->end->next == NULL);
        rz_usize capacity = RZ_ARENA_REGION_DEFAULT_CAPACITY;
        if (capacity < size) capacity = size;
        a->end->next = rz__new_region(capacity, &a->child_allocator);
        a->end       = a->end->next;
    }

    void *result = &a->end->data[a->end->count];
    a->end->count += size;
    return result;
}

void *rz__arena_remap(void *opaque, void *mem, rz_usize mem_len, rz_usize new_len) {
    RZ_DBG_ASSERT(opaque != NULL && mem != NULL && mem_len != 0);
    RZ_ArenaAllocator *a = (RZ_ArenaAllocator *)opaque;
    if (new_len <= mem_len) return mem;

    void *newptr = rz__arena_alloc(a, new_len);
    if (newptr == NULL) return NULL;
    rz_memcpy(newptr, mem, mem_len);
    return newptr;
}

void rz__arena_dealloc(void *a, void *mem, rz_usize mem_len) {
    RZ_UNUSED(a), RZ_UNUSED(mem), RZ_UNUSED(mem_len);
}

RZ_DEF RZ_Allocator rz_arena_allocator(RZ_ArenaAllocator *arena) {
    return (RZ_Allocator){.ptr = arena, .vtable = &rz__arena_allocator_vtable};
}

RZ_ArenaMark rz_arena_snapshot(RZ_ArenaAllocator *a) {
    RZ_ArenaMark m;
    if (a->end == NULL) { // snapshot of uninitialized arena
        RZ_ASSERT(a->begin == NULL);
        m.region = a->end;
        m.count  = 0;
    } else {
        m.region = a->end;
        m.count  = a->end->count;
    }

    return m;
}
void rz_arena_reset(RZ_ArenaAllocator *a) {
    for (RZ_ArenaAllocatorRegion *r = a->begin; r != NULL; r = r->next) {
        r->count = 0;
    }

    a->end = a->begin;
}

void rz_arena_rewind(RZ_ArenaAllocator *a, RZ_ArenaMark m) {
    if (m.region == NULL) { // snapshot of uninitialized arena
        rz_arena_reset(a);  // leave allocation
        return;
    }

    m.region->count = m.count;
    for (RZ_ArenaAllocatorRegion *r = m.region->next; r != NULL; r = r->next) {
        r->count = 0;
    }

    a->end = m.region;
}

void rz_arena_free(RZ_ArenaAllocator *a) {
    RZ_ArenaAllocatorRegion *r = a->begin;
    while (r) {
        RZ_ArenaAllocatorRegion *r0 = r;
        r                           = r->next;
        rz__free_region(r0, a->child_allocator);
    }
    a->begin = NULL;
    a->end   = NULL;
}

void rz_arena_trim(RZ_ArenaAllocator *a) {
    RZ_ArenaAllocatorRegion *r = a->end->next;
    while (r) {
        RZ_ArenaAllocatorRegion *r0 = r;
        r                           = r->next;
        rz__free_region(r0, a->child_allocator);
    }
    a->end->next = NULL;
}
#endif /* ifdef RZ_ALLOC_ARENA_IMPL */

#ifdef RZ_ALLOC_TEMP_IMPL
static RZ_ArenaAllocator rz__temp_arena_allocator_arena = {0};

RZ_DEF RZ_Allocator rz_temp_allocator(void) {
    if (!rz_is_allocator(rz__temp_arena_allocator_arena.child_allocator)) rz__temp_arena_allocator_arena.child_allocator = rz_std_allocator();
    return rz_arena_allocator(&rz__temp_arena_allocator_arena);
}
RZ_DEF RZ_ArenaMark rz_temp_snapshot(void) {
    return rz_arena_snapshot(&rz__temp_arena_allocator_arena);
}
RZ_DEF void rz_temp_rewind(RZ_ArenaMark m) {
    rz_arena_rewind(&rz__temp_arena_allocator_arena, m);
}
#endif

// RZ_DEF void *rz_memcpy(void *dest, const void *src, rz_usize n) {
//     rz_u8       *d = dest;
//     const rz_u8 *s = src;
//     for (; n; n--) *d++ = *s++;
//     return dest;
// }
//
// RZ_DEF void *rz_memmove(void *dest, const void *src, size_t n) {
//     char       *d = dest;
//     const char *s = src;
//
//     if (d == s) return d;
//     if ((uintptr_t)s - (uintptr_t)d - n <= -2 * n) return rz_memcpy(d, s, n);
//
//     if (d < s) {
// #ifdef __GNUC__
//         if ((uintptr_t)s % WS == (uintptr_t)d % WS) {
//             while ((uintptr_t)d % WS) {
//                 if (!n--) return dest;
//                 *d++ = *s++;
//             }
//             for (; n >= WS; n -= WS, d += WS, s += WS) *(WT *)d = *(WT *)s;
//         }
// #endif
//         for (; n; n--) *d++ = *s++;
//     } else {
// #ifdef __GNUC__
//         if ((uintptr_t)s % WS == (uintptr_t)d % WS) {
//             while ((uintptr_t)(d + n) % WS) {
//                 if (!n--) return dest;
//                 d[n] = s[n];
//             }
//             while (n >= WS) n -= WS, *(WT *)(d + n) = *(WT *)(s + n);
//         }
// #endif
//         while (n) n--, d[n] = s[n];
//     }
//
//     return dest;
// }
//
// RZ_DEF const void *rz_memchr(const void *src, int c, size_t n) {
//     const rz_u8 *s = src;
//     c              = (rz_u8)c;
//
//     for (; n && *s != c; s++, n--);
//     return n ? s : 0;
// }
//
// RZ_DEF rz_ptrdiff rz_memcmp(const void *lhs, const void *rhs, rz_usize n) {
//     const rz_u8 *l = lhs, *r = rhs;
//     for (; n && *l == *r; n--, l++, r++);
//     return n ? *l - *r : 0;
// }
//
// RZ_DEF void *rz_memset(void *dest, rz_i32 c, rz_usize n) {
//     rz_u8 *s = dest;
//     size_t k;
//
//     if (!n) return dest;
//     s[0] = c, s[n - 1] = c;
//     if (n <= 2) return dest;
//     s[1] = c, s[2] = c, s[n - 2] = c, s[n - 3] = c;
//     if (n <= 6) return dest;
//     s[3] = c, s[n - 4] = c;
//     if (n <= 8) return dest;
//
//     k = -(uintptr_t)s & 3;
//     s += k;
//     n -= k;
//     n &= -4;
//
//     for (; n; n--, s++) *s = c;
//     return dest;
// }

RZ_DEF bool rz_memequal(const void *lhs, rz_usize lhs_size, const void *rhs, rz_usize rhs_size) {
    return (lhs_size == rhs_size) && (rz_memcmp(lhs, rhs, lhs_size) == 0);
}
RZ_DEF bool rz_memswap(void *lhs, void *rhs, rz_usize n) {
    if ((lhs == NULL) || (rhs == NULL) || (n == 0)) return false;

    rz_u8 *l = lhs, *r = rhs;
    for (; n; n--, l++, r++) RZ_SWAP(*l, *r);
    return true;
}
