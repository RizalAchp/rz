#pragma once

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 202311L)
#    error "This program requires C23 (__STDC_VERSION__ >= 202311L). Compile with -std=c23 or -std=c2x."
#endif

#define RZ_IMPLEMENTATION
#ifndef RZ_COMMON_H
#    define RZ_COMMON_H

#    ifdef RZ_IMPLEMENTATION
#        define RZ_ALLOC_IMPL
#        define RZ_ALLOC_ALL_IMPL

#        define RZ_COLLECTIONS_IMPL
#        define RZ_STRING_IMPL
#        define RZ_SPRINTF_IMPL
#        define RZ_ARGPARSE_IMPL
#    endif

#    ifdef RZ_ARGPARSE_IMPL
#        ifndef RZ_HASHMAP_IMPL
#            define RZ_HASHMAP_IMPL
#        endif
#    endif

#    ifdef RZ_COLLECTIONS_IMPL
#        define RZ_ARRAY_IMPL
#        define RZ_HASHMAP_IMPL
#        define RZ_BTREE_IMPL
#    endif /* ifdef RZ_COLLECTIONS_IMPL */

#    ifdef RZ_HASHMAP_IMPL
#        ifndef RZ_ARRAY_IMPL
#            define RZ_ARRAY_IMPL
#        endif
#    endif

#    ifdef RZ_BTREE_IMPL
#        ifndef RZ_ARRAY_IMPL
#            define RZ_ARRAY_IMPL
#        endif
#    endif

#    ifdef RZ_STRING_IMPL
#        ifndef RZ_ARRAY_IMPL
#            define RZ_ARRAY_IMPL
#        endif
#    endif

#    ifdef RZ_ARRAY_IMPL
#        define RZ_ALLOC_IMPL
#    endif

#    ifdef RZ_ALLOC_ALL_IMPL
#        ifndef RZ_ALLOC_IMPL
#            define RZ_ALLOC_IMPL
#        endif
#        ifndef RZ_ALLOC_STD_IMPL
#            define RZ_ALLOC_STD_IMPL
#        endif
#        ifndef RZ_ALLOC_PAGE_IMPL
#            define RZ_ALLOC_PAGE_IMPL
#        endif
#        ifndef RZ_ALLOC_ARENA_IMPL
#            define RZ_ALLOC_ARENA_IMPL
#        endif
#        ifndef RZ_ALLOC_TEMP_IMPL
#            define RZ_ALLOC_TEMP_IMPL
#        endif
#    endif

#    ifdef RZ_ALLOC_STD_IMPL
#        ifndef RZ_ALLOC_IMPL
#            define RZ_ALLOC_IMPL
#        endif
#    endif

#    ifdef RZ_ALLOC_PAGE_IMPL
#        ifndef RZ_ALLOC_IMPL
#            define RZ_ALLOC_IMPL
#        endif
#    endif

#    ifdef RZ_ALLOC_TEMP_IMPL
#        ifndef RZ_ALLOC_ARENA_IMPL
#            define RZ_ALLOC_ARENA_IMPL
#        endif
#        ifndef RZ_ALLOC_IMPL
#            define RZ_ALLOC_IMPL
#        endif
#    endif

#    ifdef RZ_ALLOC_ARENA_IMPL
#        ifndef RZ_ALLOC_IMPL
#            define RZ_ALLOC_IMPL
#        endif
#    endif

#    ifdef RZ_SPRINTF_IMPL
#    endif

#    ifndef RZ_DEF
#        define RZ_DEF
#    endif
#    ifndef RZ_DEC
#        define RZ_DEC extern
#    endif

/* Compiler detection */
#    if defined(_MSC_VER)
#        define RZ_CC_MSVC
#    elif (defined(__MINGW32__) || defined(__MINGW64__))
#        define RZ_CC_MINGW
#    elif defined(__clang__)
#        define RZ_CC_CLANG
#    elif (defined(__GNUC__) || defined(__GNUG__))
#        define RZ_CC_GCC
#    else
#        error "Unknown compiler. Only support clang, gcc, msvc, and mingw"
#    endif

#    if defined(_WIN32)
#        define RZ_OS_WINDOWS
#    elif defined(__EMSCRIPTEN__)
#        define RZ_OS_EMSCRIPTEN
#    elif defined(__HAIKU__)
#        define RZ_OS_HAIKU
#    elif defined(__wasi__)
#        define RZ_OS_WASI
#    elif defined(__unix)
#        define RZ_OS_UNIX
#        if defined(__APPLE__)
#            define RZ_OS_APPLE
#        endif
#        if defined(__FreeBSD__)
#            define RZ_OS_FREEBSD
#        endif
#        if defined(__NetBSD__)
#            define RZ_OS_NETBSD
#        endif
#        if defined(__OpenBSD__)
#            define RZ_OS_OPENBSD
#        endif
#        if defined(__linux__)
#            define RZ_OS_LINUX
#        endif
#    else
#        error "Unknown OS. unsupported OS"
#    endif

#    if (defined(__x86_64__) || defined(_M_X64))
#        define RZ_ARCH_TAG  RZ_ARCH_X86_64
#        define RZ_ARCH_BITS 64
#    elif (defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86))
#        define RZ_ARCH_TAG  RZ_ARCH_X86
#        define RZ_ARCH_BITS 32
#    elif (defined(__aarch64__) || defined(_M_ARM64))
#        define RZ_ARCH_TAG  RZ_ARCH_AARCH64
#        define RZ_ARCH_BITS 64
#    elif (defined(__arm__) || defined(_M_ARM))
#        define RZ_ARCH_TAG  RZ_ARCH_ARM
#        define RZ_ARCH_BITS 32
#    elif (defined(__mips64__) || defined(__mips64))
#        define RZ_ARCH_TAG  RZ_ARCH_MIPS64
#        define RZ_ARCH_BITS 64
#    elif (defined(mips) || defined(__mips__) || defined(__mips))
#        define RZ_ARCH_TAG  RZ_ARCH_MIPS
#        define RZ_ARCH_BITS 32
#    elif (defined(__PPC64__) || defined(__ppc64__) || defined(_ARCH_PPC64))
#        define RZ_ARCH_TAG  RZ_ARCH_POWERPC64
#        define RZ_ARCH_BITS 64
#    elif (defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__) || defined(__ppc__) || defined(__PPC__) || defined(_ARCH_PPC))
#        define RZ_ARCH_TAG  RZ_ARCH_POWERPC
#        define RZ_ARCH_BITS 32
#    elif defined(__wasm64__)
#        define RZ_ARCH_TAG  RZ_ARCH_WASM64
#        define RZ_ARCH_BITS 64
#    elif defined(__wasm32__)
#        define RZ_ARCH_TAG  RZ_ARCH_WASM32
#        define RZ_ARCH_BITS 32
#    elif (defined(__riscv) && (__riscv_xlen == 64))
#        define RZ_ARCH_TAG  RZ_ARCH_RISCV64
#        define RZ_ARCH_BITS 64
#    elif (defined(__riscv) && (__riscv_xlen == 32))
#        define RZ_ARCH_TAG  RZ_ARCH_RISCV32
#        define RZ_ARCH_BITS 32
#    else
#        define RZ_ARCH_TAG RZ_ARCH_UNKNOWN
#        error "Unknown Architecture. unsupported!"
#    endif

#    if (RZ_ARCH_BITS == 64)
#        define RZ_BIT64
#    elif (RZ_ARCH_BITS == 32)
#        define RZ_BIT32
#    else
#        error "Unknown Bits. only supported (64bit and 32bit) "
#    endif

#    include <limits.h>
#    include <stdalign.h>
#    include <stdarg.h>
#    include <stdbool.h>
#    include <stddef.h>
#    include <stdint.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <threads.h>

#    if defined(RZ_OS_WINDOWS)
#        define WIN32_LEAN_AND_MEAN
#        define _CRT_SECURE_NO_WARNINGS
// #        define _WINUSER_
// #        define _WINGDI_
// #        define _IMM_
// #        define _WINCON_
#        include <intrin.h>
#        include <windows.h>

#        include <direct.h>
#        include <io.h>
#        include <shellapi.h>

#        define RZ_ENDLINE "\r\n"
#    else
#        define RZ_ENDLINE "\n"
#    endif

#    if defined(RZ_OS_UNIX)
#        include <dirent.h>
#        include <fcntl.h>
#        include <sys/stat.h>
#        include <sys/types.h>
#        include <sys/wait.h>
#        include <unistd.h>
#    endif

#    if defined(RZ_OS_HAIKU)
#        include <image.h>
#    endif

#    if defined(RZ_OS_APPLE)
#        include <mach-o/dyld.h>
#    endif
#    if defined(RZ_OS_FREEBSD)
#        include <sys/sysctl.h>
#    endif

#    define rz_u8        uint8_t
#    define rz_i8        int8_t
#    define rz_u16       uint16_t
#    define rz_i16       int16_t
#    define rz_u32       uint32_t
#    define rz_i32       int32_t
#    define rz_u64       uint64_t
#    define rz_i64       int64_t
#    define rz_f64       double
#    define rz_f32       float
#    define rz_usize     size_t
#    define rz_isize     ptrdiff_t
#    define rz_ptrdiff   ptrdiff_t
#    define rz_uptr      uintptr_t
#    define rz_iptr      intptr_t
#    define rz_char      char
#    define rz_int       int

#    define RZ_U8_MIN    0
#    define RZ_U8_MAX    UINT8_MAX
#    define RZ_I8_MIN    INT8_MIN
#    define RZ_I8_MAX    INT8_MAX
#    define RZ_U16_MIN   0
#    define RZ_U16_MAX   UINT16_MAX
#    define RZ_I16_MIN   INT16_MIN
#    define RZ_I16_MAX   INT16_MAX
#    define RZ_U32_MIN   0
#    define RZ_U32_MAX   UINT32_MAX
#    define RZ_I32_MIN   INT32_MIN
#    define RZ_I32_MAX   INT32_MAX
#    define RZ_U64_MIN   0
#    define RZ_U64_MAX   UINT64_MAX
#    define RZ_I64_MIN   INT64_MIN
#    define RZ_I64_MAX   INT64_MAX

#    define RZ_MAX(A, B) (((A) > (B)) ? (A) : (B))
#    define RZ_MIN(A, B) (((A) < (B)) ? (A) : (B))

#    if defined(__has_attribute)
#        define RZ_HAS_ATTR __has_attribute
#    else
#        define RZ_HAS_ATTR(...) 0
#    endif

#    if defined(__has_builtin)
#        define RZ_HAS_BUILTIN __has_builtin
#    else
#        define RZ_HAS_BUILTIN(...) 0
#    endif

#    if !defined(RZ_CC_MSVC)
#        define RZ_ATTR __attribute__
#    else
#        define RZ_ATTR(...)
#    endif

#    ifdef RZ_BUILD_DEBUG
#        define RZ_DEBUG 1
#    else
#        if (defined(DEBUG) || defined(_DEBUG)) || !defined(NDEBUG) || (!defined(NDEBUG) && !defined(__OPTIMIZE__))
#            define RZ_DEBUG 1
#        else
#            define RZ_DEBUG 0
#        endif
#    endif

#    if defined(RZ_CC_MSVC)
#        define RZ_NODISCARD _Check_return_
#    else
#        define RZ_NODISCARD [[nodiscard]]
#    endif

#    if defined(RZ_CC_MSVC)
#        define RZ_NORETURN __declspec(noreturn)
#    else
#        define RZ_NORETURN [[noreturn]]
#    endif

#    if defined(RZ_CC_MSVC)
#        define RZ_NOINLINE __declspec(noinline)
#    elif (RZ_HAS_ATTR(noinline))
#        define RZ_NOINLINE RZ_ATTR((noinline))
#    elif (defined(RZ_CC_GCC) || defined(RZ_CC_CLANG))
#        define RZ_NOINLINE RZ_ATTR((noinline))
#    else
#        define RZ_NOINLINE
#    endif

#    if defined(RZ_CC_MSVC)
#        define RZ_PRINTF_FMT(FMT) _Printf_format_string_ FMT
#        define RZ_PRINTF_FORMAT(...)
#    elif (RZ_HAS_ATTR(format))
#        define RZ_PRINTF_FMT(FMT) FMT
#        if defined(__MINGW_PRINTF_FORMAT)
#            define RZ_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) RZ_ATTR((format(__MINGW_PRINTF_FORMAT, STRING_INDEX, FIRST_TO_CHECK)))
#        else
#            define RZ_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) RZ_ATTR((format(printf, STRING_INDEX, FIRST_TO_CHECK)))
#        endif
#    endif                      // if defined(RZ_CC_MSVC)
#    if (!defined(RZ_PRINTF_FORMAT))
#        define RZ_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#    endif                      // if (!defined(RZ_PRINTF_FORMAT))
#    if (!defined(RZ_PRINTF_FMT))
#        define RZ_PRINTF_FMT(FMT) FMT
#    endif                      // if (!defined(RZ_PRINTF_FMT))

#    if (defined(RZ_WARN_DEPRECATED) && !defined(RZ_DEPRECATED))
#        if defined(RZ_CC_MSVC) // https://en.cppreference.com/c/compiler_support/23
#            define RZ_DEPRECATED(message) __declspec(deprecated(message))
#        elif (defined(RZ_CC_GCC) || defined(RC_CC_CLANG))
#            define RZ_DEPRECATED(message) [[deprecated(message)]]
#        else
#            define RZ_DEPRECATED(...)
#        endif
#    else
#        define RZ_DEPRECATED(...)
#    endif // if (defined(RZ_WARN_DEPRECATED) && !defined(RZ_DEPRECATED))

#    if defined(RZ_CC_MSVC)
#        define RZ_UNUSED(value) (void)(value)
#    else
#        define RZ_UNUSED(value) (void)sizeof(value)
#    endif // if defined(RZ_CC_MSVC)

#    if (defined(__cplusplus))
#        define RZ_HAS_TYPEOF
#        define RZ_TYPEOF(x)      declspec(x)
#        define RZ_EXTERN_C       extern "C"
#        define RZ_EXTERN_C_BEGIN extern "C" {
#        define RZ_EXTERN_C_END   }
#    else
#        define RZ_HAS_TYPEOF
#        define RZ_TYPEOF(x) typeof(x)
#        define RZ_EXTERN_C_BEGIN
#        define RZ_EXTERN_C_END
#        define RZ_EXTERN_C
#    endif                                                                    // if defined(__cplusplus)

#    if defined(RZ_HAS_TYPEOF)
#        define RZ_ADDRESSOF(typevar, value) ((RZ_TYPEOF(typevar)[1]){value}) // literal array decays to pointer to value
#    else
#        define RZ_ADDRESSOF(typevar, value) &(value)
#    endif                                                                    // if defined(RZ_HAS_TYPEOF)
#    define RZ_OFFSETOF(var, field) offsetof(var, field)

#    if (RZ_HAS_BUILTIN(__builtin_types_compatible_p) && defined(RZ_HAS_TYPEOF))
#        define RZ_TYPES_COMPATIBLE(A, B) (__builtin_types_compatible_p(RZ_TYPEOF(A), RZ_TYPEOF(B)))
#    else
#        define RZ_TYPES_COMPATIBLE(A, B) (_Generic(A, RZ_TYPEOF(B): true, default: false))
#    endif // if (RZ_HAS_BUILTIN(__builtin_types_compatible_p) && defined(RZ_HAS_TYPEOF))

#    define RZ_STATIC_ASSERT                       static_assert
#    define RZ_STATIC_ASSERT_TYPE_COMPATIBLE(A, B) RZ_STATIC_ASSERT(RZ_TYPES_COMPATIBLE(A, B), "typeof (" RZ_STRINGIFY(A) ") and (" RZ_STRINGIFY(B) ") is not compatible")

#    define RZ_ASSERT(EXPR, ...)                   ((void)((!!(EXPR)) || (RZ_PANIC("'" #EXPR "' - " __VA_ARGS__), 0)))
#    define RZ_ASSERT_MSG(C, MSG)                  RZ_ASSERT((C), (MSG))
#    define RZ_ASSERT_NOT_NULL(PTR)                RZ_ASSERT((PTR) != NULL && "Ptr should not be NULL")
#    define RZ_ASSERT_ALLOCATOR_PTR(PTR)           RZ_ASSERT((PTR) != NULL && "Allocator Return NULL. Memory Full?")

#    if RZ_DEBUG
#        define RZ_DBG_ASSERT(EXPR, ...) RZ_ASSERT(EXPR, "DEBUG ASSERTION: " __VA_ARGS__)
#    else
#        define RZ_DBG_ASSERT(...)
#    endif // if RZ_DEBUG
#    define RZ_DBG_ASSERT_MSG(EXPR, MSG)     RZ_ASSERT(EXPR, "DEBUG ASSERTION: " MSG)
#    define RZ_DBG_ASSERT_NOT_NULL(PTR)      RZ_ASSERT((PTR) != NULL && "Ptr should not be NULL")
#    define RZ_DBG_ASSERT_ALLOCATOR_PTR(PTR) RZ_ASSERT((PTR) != NULL && "Allocator Return NULL. Memory Full?")

#    ifndef RZ_ALLOC_STACK
#        if defined(RZ_CC_MSVC)
#            define RZ_ALLOC_STACK(size) _alloca(size)
#        elif (defined(RZ_CC_GCC) || defined(RZ_CC_CLANG))
#            define RZ_ALLOC_STACK(size) alloca(size)
#        elif (defined(__has_builtin) && __has_builtin(__builtin_alloca))
#            define RZ_ALLOC_STACK(size) __builtin_alloca(size)
#        else
#            define RZ_ALLOC_STACK(size) RZ_STATIC_ASSERT(false, "try to `alloca` allocate with stack but no function available")
#        endif /* ifdef RZ_CC_MSVC */
#    endif     /* ifndef RZ_ALLOC_STACK */

// clang-format off
#    define RZ_STRINGIFY_VALUE(ITEM)        RZ_STRINGIFY(ITEM)
#    define RZ_STRINGIFY(ITEM)              #ITEM

#    define RZ_LOCATION                     __FILE__ ":" RZ_STRINGIFY_VALUE(__LINE__) ":"

#    define RZ_PANIC(...)                   rz_panic_impl(RZ_LOCATION, __VA_ARGS__)
#    define RZ_TODO(...)                    RZ_PANIC("TODO: " __VA_ARGS__)
#    define RZ_UNREACHABLE(...)             RZ_PANIC("UNREACHABLE: " __VA_ARGS__)
#    define RZ_UNIMPLEMENTED(...)           RZ_PANIC("UNIMPLEMENTED: " __VA_ARGS__)

#    define RZ_ARRAY_LEN(array)             (sizeof(array) / sizeof(array[0]))
#    define RZ_ARRAY_GET(array, index)      array[(RZ_ASSERT((size_t)index < RZ_ARRAY_LEN(array)), (size_t)index)]
#    define RZ_SWAP(A, B)                   do { RZ_TYPEOF(A) _t = (A); (A) = (B); (B) = _t; } while (0)
#    define rz_return_defer(value)          do { result = (value); goto defer; } while (0)

//   RUST?? xd
#    define RZ_Opt(T)                       struct { T unwrap; bool is_some; }
#    define RZ_Result(T, E)                 struct { T unwrap; E error; bool is_ok; }

#    define rz_opt_eq(lhs_opt, rhs_opt)      (((lhs_opt).is_some == false  && (rhs_opt).is_some == false) || rz_opt_some_eq(lhs_opt, rhs_opt))
#    define rz_opt_some_eq(lhs_opt, rhs_opt) ((lhs_opt).is_some && (rhs_opt).is_some && (lhs_opt).unwrap == (rhs_opt).unwrap)
#    define rz_opt_is_some_eq(opt, value)    ((opt).is_some && (opt).unwrap == (value))

#    define rz_opt_unwrap(opt)              (RZ_ASSERT(opt.is_some),     opt.unwrap)
#    define rz_result_unwrap(result)        (RZ_ASSERT(result.is_ok), result.unwrap)
// clang-format on

RZ_EXTERN_C_BEGIN

enum
{
    RZ_ARCH_UNKNOWN = 0,
    RZ_ARCH_X86,    // 32bit
    RZ_ARCH_X86_64, // 64bit
    RZ_ARCH_ARM,
    RZ_ARCH_AARCH64,
    RZ_ARCH_MIPS,
    RZ_ARCH_MIPS64,
    RZ_ARCH_POWERPC,
    RZ_ARCH_POWERPC64,
    RZ_ARCH_WASM32,
    RZ_ARCH_WASM64,
    RZ_ARCH_RISCV32,
    RZ_ARCH_RISCV64,
};

#    ifndef RZ_WINDOWS_MSGBOX_ERROR
#    endif /* ifndef RZ_WINDOWS_MSGBOX_ERROR */

#    if defined(RZ_OS_WINDOWS)
#        define rz_errno GetLastError()
#    else
#        include <errno.h>
#        define rz_errno errno
#    endif
#    ifndef MAX_BUFFER_STRERROR_LEN
#        define MAX_BUFFER_STRERROR_LEN (4u * 1024u)
#    endif

RZ_DEF const char      *rz_strerror(void);
RZ_DEF RZ_NORETURN void rz_panic_impl(const char *loc, RZ_PRINTF_FMT(const char *fmt), ...) RZ_PRINTF_FORMAT(2, 3);

#    define RZ_ANSI_RESET          "\x1b[0m"
#    define RZ_ANSI_BLACK          "\x1b[30m"
#    define RZ_ANSI_RED            "\x1b[31m"
#    define RZ_ANSI_GREEN          "\x1b[32m"
#    define RZ_ANSI_YELLOW         "\x1b[33m"
#    define RZ_ANSI_BLUE           "\x1b[34m"
#    define RZ_ANSI_MAGENTA        "\x1b[35m"
#    define RZ_ANSI_CYAN           "\x1b[36m"
#    define RZ_ANSI_WHITE          "\x1b[37m"
#    define RZ_ANSI_BRIGHT_BLACK   "\x1b[90m"
#    define RZ_ANSI_BRIGHT_RED     "\x1b[91m"
#    define RZ_ANSI_BRIGHT_GREEN   "\x1b[92m"
#    define RZ_ANSI_BRIGHT_YELLOW  "\x1b[93m"
#    define RZ_ANSI_BRIGHT_BLUE    "\x1b[94m"
#    define RZ_ANSI_BRIGHT_MAGENTA "\x1b[95m"
#    define RZ_ANSI_BRIGHT_CYAN    "\x1b[96m"
#    define RZ_ANSI_BRIGHT_WHITE   "\x1b[97m"
RZ_DEF bool rz_isatty(FILE *f);
RZ_EXTERN_C_END
#endif /* end of include guard: RZ_COMMON_H */
