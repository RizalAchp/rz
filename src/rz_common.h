#pragma once

#ifndef RZ_COMMON_H
#    define RZ_COMMON_H

#    if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 202311L)
#        error "This program requires C23 (__STDC_VERSION__ >= 202311L). Compile with -std=c23 or -std=c2x."
#    endif
#    include "template.rz.h"
#    ifndef RZ_DEF
#        define RZ_DEF
#    endif
#    ifndef RZ_DEC
#        define RZ_DEC extern
#    endif

// clang-format off
#    define RZ_0(sep, m)
#    define RZ_1(sep, m, a)       m(a)
#    define RZ_2(sep, m, a, ...)  m(a) sep RZ_1(sep, m, __VA_ARGS__)
#    define RZ_3(sep, m, a, ...)  m(a) sep RZ_2(sep, m, __VA_ARGS__)
#    define RZ_4(sep, m, a, ...)  m(a) sep RZ_3(sep, m, __VA_ARGS__)
#    define RZ_5(sep, m, a, ...)  m(a) sep RZ_4(sep, m, __VA_ARGS__)
#    define RZ_6(sep, m, a, ...)  m(a) sep RZ_5(sep, m, __VA_ARGS__)
#    define RZ_7(sep, m, a, ...)  m(a) sep RZ_6(sep, m, __VA_ARGS__)
#    define RZ_8(sep, m, a, ...)  m(a) sep RZ_7(sep, m, __VA_ARGS__)
#    define RZ_9(sep, m, a, ...)  m(a) sep RZ_8(sep, m, __VA_ARGS__)
#    define RZ_10(sep, m, a, ...) m(a) sep RZ_9(sep, m, __VA_ARGS__)
#    define RZ_11(sep, m, a, ...) m(a) sep RZ_10(sep, m, __VA_ARGS__)
#    define RZ_12(sep, m, a, ...) m(a) sep RZ_11(sep, m, __VA_ARGS__)
#    define RZ_13(sep, m, a, ...) m(a) sep RZ_12(sep, m, __VA_ARGS__)
#    define RZ_14(sep, m, a, ...) m(a) sep RZ_13(sep, m, __VA_ARGS__)
#    define RZ_15(sep, m, a, ...) m(a) sep RZ_14(sep, m, __VA_ARGS__)
#    define RZ_16(sep, m, a, ...) m(a) sep RZ_15(sep, m, __VA_ARGS__)
#    define RZ_17(sep, m, a, ...) m(a) sep RZ_16(sep, m, __VA_ARGS__)
#    define RZ_18(sep, m, a, ...) m(a) sep RZ_17(sep, m, __VA_ARGS__)
#    define RZ_19(sep, m, a, ...) m(a) sep RZ_18(sep, m, __VA_ARGS__)
#    define RZ_20(sep, m, a, ...) m(a) sep RZ_19(sep, m, __VA_ARGS__)
#    define RZ_NARGS(...)         RZ_NARGS_IMPL(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#    define RZ_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
// clang-format on
#    define RZ_CONCAT(a, b)             RZ_CONCAT2(a, b)
#    define RZ_CONCAT2(a, b)            a##b
#    define RZ_FOR_EACH(sep, func, ...) RZ_CONCAT(RZ_, RZ_NARGS(__VA_ARGS__))(sep, func, __VA_ARGS__)

#    define RZ_NOP(A)                   A
#    define RZ_ANY(F, ...)              RZ_FOR_EACH(||, F, __VA_ARGS__)
#    define RZ_ALL(F, ...)              RZ_FOR_EACH(&&, F, __VA_ARGS__)

#    ifdef RZ_BUILD_DEBUG
#        define RZ_DEBUG 1
#    else
#        if (defined(DEBUG) || defined(_DEBUG)) || !defined(NDEBUG) || (!defined(NDEBUG) && !defined(__OPTIMIZE__))
#            define RZ_DEBUG 1
#        else
#            define RZ_DEBUG 0
#        endif
#    endif

#    if (defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64))
#        define RZ_TARGET_BITS_64     1
#        define RZ_TARGET_ARCH_X86_64 1
#    elif (defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(_X86_) || defined(__X86__))
#        define RZ_TARGET_BITS_32  1
#        define RZ_TARGET_ARCH_X86 1
#    elif (defined(__aarch64__) || defined(_M_ARM64))
#        define RZ_TARGET_BITS_64      1
#        define RZ_TARGET_ARCH_AARCH64 1
#    elif (defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_M_ARM) || defined(__arm))
#        define RZ_TARGET_BITS_32  1
#        define RZ_TARGET_ARCH_ARM 1
#        if (defined(__thumb__) || defined(__TARGET_ARCH_THUMB) || defined(_M_ARMT))
#            define RZ_TARGET_ARCH_ARM_THUMB 1
#        endif
#    elif (defined(__riscv) || defined(__riscv_xlen))
#        if __riscv_xlen == 32
#            define RZ_TARGET_BITS_32      1
#            define RZ_TARGET_ARCH_RISCV32 1
#        else
#            define RZ_TARGET_BITS_64      1
#            define RZ_TARGET_ARCH_RISCV64 1
#        endif
#    elif (defined(__PPC64__) || defined(__ppc64__) || defined(__powerpc64__) || defined(_ARCH_PPC64))
#        define RZ_TARGET_BITS_64        1
#        define RZ_TARGET_ARCH_POWERPC64 1
#    elif (defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__) || defined(__ppc__) || defined(__PPC__) || defined(_ARCH_PPC) || defined(_M_PPC))
#        define RZ_TARGET_BITS_32        1
#        define RZ_TARGET_ARCH_POWERPC32 1
#    elif defined(__wasm64__)
#        define RZ_TARGET_BITS_64     1
#        define RZ_TARGET_ARCH_WASM64 1
#    elif defined(__wasm32__)
#        define RZ_TARGET_BITS_32     1
#        define RZ_TARGET_ARCH_WASM32 1
#    endif
#    ifndef RZ_TARGET_BITS_32
#        define RZ_TARGET_BITS_32 0
#    endif
#    ifndef RZ_TARGET_BITS_64
#        define RZ_TARGET_BITS_64 0
#    endif
#    ifndef RZ_TARGET_ARCH_X86_64
#        define RZ_TARGET_ARCH_X86_64 0
#    endif
#    ifndef RZ_TARGET_ARCH_X86
#        define RZ_TARGET_ARCH_X86 0
#    endif
#    ifndef RZ_TARGET_ARCH_ARM
#        define RZ_TARGET_ARCH_ARM 0
#    endif
#    ifndef RZ_TARGET_ARCH_ARM_THUMB
#        define RZ_TARGET_ARCH_ARM_THUMB 0
#    endif
#    ifndef RZ_TARGET_ARCH_AARCH64
#        define RZ_TARGET_ARCH_AARCH64 0
#    endif
#    ifndef RZ_TARGET_ARCH_RISCV64
#        define RZ_TARGET_ARCH_RISCV64 0
#    endif
#    ifndef RZ_TARGET_ARCH_RISCV32
#        define RZ_TARGET_ARCH_RISCV32 0
#    endif
#    ifndef RZ_TARGET_ARCH_POWERPC64
#        define RZ_TARGET_ARCH_POWERPC64 0
#    endif
#    ifndef RZ_TARGET_ARCH_POWERPC32
#        define RZ_TARGET_ARCH_POWERPC32 0
#    endif
#    ifndef RZ_TARGET_ARCH_WASM64
#        define RZ_TARGET_ARCH_WASM64 0
#    endif
#    ifndef RZ_TARGET_ARCH_WASM32
#        define RZ_TARGET_ARCH_WASM32 0
#    endif

#    if defined(_WIN64) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#        define RZ_TARGET_FAMILY_WINDOWS 1
#        define RZ_TARGET_OS_WINDOWS     1
#        include <KnownFolders.h>
#        include <direct.h>
#        include <intrin.h>
#        include <intsafe.h>
#        include <io.h>
#        include <shellapi.h>
#        include <windows.h>
#        define RZ_ENDLINE "\r\n"
#    elif defined(EMSCRIPTEN) || defined(__EMSCRIPTEN__)
#        define RZ_TARGET_FAMILY_WASM         1
#        define RZ_TARGET_OS_UNKNOWN          1
#        define RZ_TARGET_COMPILER_EMSCRIPTEN 1
#    elif defined(__wasi__)
#        define RZ_TARGET_FAMILY_WASM   1
#        define RZ_TARGET_OS_UNKNOWN    1
#        define RZ_TARGET_COMPILER_WASI 1
#    elif defined(__APPLE__)
#        define RZ_TARGET_FAMILY_UNIX  1
#        define RZ_TARGET_FAMILY_APPLE 1
#        include "TargetConditionals.h"
#        if TARGET_OS_IPHONE
#            define RZ_TARGET_OS_IOS 1
#            // define something for iphone
#        elif TARGET_OS_MAC
#            define RZ_TARGET_OS_MACOS 1
#            include <mach-o/dyld.h>
#        else
#            error apple target is not implemented yet or not supported
#        endif
#        include <libproc.h>
#    elif (defined(__unix) || defined(__unix__))
#        define RZ_TARGET_FAMILY_UNIX 1
#        if defined(__FreeBSD__)
#            define RZ_TARGET_OS_FREEBSD 1
#        elif defined(__NetBSD__)
#            define RZ_TARGET_OS_NETBSD 1
#        elif defined(__OpenBSD__)
#            define RZ_TARGET_OS_OPENBSD 1
#        elif defined(__ANDROID__)
#            define RZ_TARGET_OS_ANDROID 1
#        elif (defined(__linux__) || defined(linux) || defined(__linux))
#            define RZ_TARGET_OS_LINUX 1
#        endif
#    else
#        error "Unknown OS. unsupported OS"
#    endif
#    ifndef RZ_TARGET_FAMILY_WINDOWS
#        define RZ_TARGET_FAMILY_WINDOWS 0
#    endif
#    ifndef RZ_TARGET_FAMILY_WASM
#        define RZ_TARGET_FAMILY_WASM 0
#    endif
#    ifndef RZ_TARGET_FAMILY_APPLE
#        define RZ_TARGET_FAMILY_APPLE 0
#    endif
#    ifndef RZ_TARGET_FAMILY_UNIX
#        define RZ_TARGET_FAMILY_UNIX 0
#    endif
#    ifndef RZ_TARGET_OS_WINDOWS
#        define RZ_TARGET_OS_WINDOWS 0
#    endif
#    ifndef RZ_TARGET_OS_MACOS
#        define RZ_TARGET_OS_MACOS 0
#    endif
#    ifndef RZ_TARGET_OS_IOS
#        define RZ_TARGET_OS_IOS 0
#    endif
#    ifndef RZ_TARGET_OS_LINUX
#        define RZ_TARGET_OS_LINUX 0
#    endif
#    ifndef RZ_TARGET_OS_ANDROID
#        define RZ_TARGET_OS_ANDROID 0
#    endif
#    ifndef RZ_TARGET_OS_FREEBSD
#        define RZ_TARGET_OS_FREEBSD 0
#    endif
#    ifndef RZ_TARGET_OS_OPENBSD
#        define RZ_TARGET_OS_OPENBSD 0
#    endif
#    ifndef RZ_TARGET_OS_NETBSD
#        define RZ_TARGET_OS_NETBSD 0
#    endif

/* Compiler detection */
#    if defined(_MSC_VER) || defined(_MSC_BUILD)
#        define RZ_TARGET_COMPILER_MSVC 1
#    elif (defined(__MINGW32__) || defined(__MINGW64__))
#        define RZ_TARGET_COMPILER_MINGW 1
#    elif defined(__CYGWIN__)
#        define RZ_TARGET_COMPILER_CYGWIN 1
#    elif defined(__clang__) || defined(__clang_version__)
#        define RZ_TARGET_COMPILER_CLANG 1
#    elif (defined(__GNUC__) || defined(__GNUC_MINOR__) || defined(__GNUC_PATCHLEVEL__))
#        define RZ_TARGET_COMPILER_GCC 1
#    else
#        error "Unsupprted compiler. Only support CLANG, GCC, MSVC, MINGW, and CYGWIN"
#    endif
#    ifndef RZ_TARGET_COMPILER_EMSCRIPTEN
#        define RZ_TARGET_COMPILER_EMSCRIPTEN 0
#    endif
#    ifndef RZ_TARGET_COMPILER_WASI
#        define RZ_TARGET_COMPILER_WASI 0
#    endif
#    ifndef RZ_TARGET_COMPILER_MSVC
#        define RZ_TARGET_COMPILER_MSVC 0
#    endif
#    ifndef RZ_TARGET_COMPILER_MINGW
#        define RZ_TARGET_COMPILER_MINGW 0
#    endif
#    ifndef RZ_TARGET_COMPILER_CYGWIN
#        define RZ_TARGET_COMPILER_CYGWIN 0
#    endif
#    ifndef RZ_TARGET_COMPILER_CLANG
#        define RZ_TARGET_COMPILER_CLANG 0
#    endif
#    ifndef RZ_TARGET_COMPILER_GCC
#        define RZ_TARGET_COMPILER_GCC 0
#    endif

#    define RZ_TARGET_BITS(A)       RZ_TARGET_BITS_##A
#    define RZ_TARGET_ARCH(A)       RZ_TARGET_ARCH_##A
#    define RZ_TARGET_OS(A)         RZ_TARGET_OS_##A
#    define RZ_TARGET_FAMILY(A)     RZ_TARGET_FAMILY_##A
#    define RZ_TARGET_COMPILER(A)   RZ_TARGET_COMPILER_##A

#    define RZ_TARGET(TGT, V)       RZ_TARGET_##TGT(V)
#    define RZ_TARGET_ANY(TGT, ...) (RZ_ANY(RZ_TARGET_##TGT, __VA_ARGS__))
#    define RZ_TARGET_ALL(TGT, ...) (RZ_ALL(RZ_TARGET_##TGT, __VA_ARGS__))

#    ifndef RZ_ENDLINE
#        define RZ_ENDLINE "\n"
#    endif
#    if RZ_TARGET_FAMILY_UNIX
#        include <unistd.h>
#
#        include <dirent.h>
#        include <fcntl.h>
#        include <malloc.h>
#        include <pwd.h>
#        include <sys/stat.h>
#        include <sys/types.h>
#        include <sys/wait.h>
#    endif
#    if RZ_TARGET_OS_LINUX
#        include <sys/sendfile.h>
#    endif
#    if RZ_TARGET_OS_FREEBSD
#        include <sys/sysctl.h>
#    endif

#    include <ctype.h>
#    include <errno.h>
#    include <limits.h>
#    include <stdalign.h>
#    include <stdarg.h>
#    include <stdatomic.h>
#    include <stdbool.h>
#    include <stddef.h>
#    include <stdint.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>
#    include <threads.h>

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
#    if !RZ_TARGET_COMPILER_MSVC
#        define RZ_ATTR __attribute__
#    else
#        define RZ_ATTR(...)
#    endif
#    if RZ_TARGET_COMPILER_MSVC
#        define RZ_NODISCARD _Check_return_
#    else
#        define RZ_NODISCARD [[nodiscard]]
#    endif
#    if RZ_TARGET_COMPILER_MSVC
#        define RZ_NORETURN __declspec(noreturn)
#    else
#        define RZ_NORETURN [[noreturn]]
#    endif
#    if RZ_TARGET_COMPILER_MSVC
#        define RZ_NOINLINE __declspec(noinline)
#    elif (RZ_HAS_ATTR(noinline))
#        define RZ_NOINLINE RZ_ATTR((noinline))
#    elif RZ_TARGET_ANY(COMPILER, GCC, CLANG)
#        define RZ_NOINLINE RZ_ATTR((noinline))
#    else
#        define RZ_NOINLINE
#    endif
#    if RZ_TARGET_COMPILER_MSVC
#        define RZ_ALWAYS_INLINE __forceinline
#    elif RZ_TARGET_COMPILER_CLANG
#        define RZ_ALWAYS_INLINE [[clang::always_inline]]
#    elif RZ_TARGET_COMPILER_GCC
#        define RZ_ALWAYS_INLINE RZ_ATTR((always_inline))
#    elif RZ_HAS_ATTR(always_inline)
#        define RZ_ALWAYS_INLINE RZ_ATTR((always_inline))
#    else
#        define RZ_ALWAYS_INLINE inline
#    endif
#    if RZ_TARGET_COMPILER_MSVC
#        define RZ_PRINTF_FMT(FMT) _Printf_format_string_ FMT
#        define RZ_PRINTF_FORMAT(...)
#    elif (RZ_HAS_ATTR(format))
#        define RZ_PRINTF_FMT(FMT) FMT
#        if defined(__MINGW_PRINTF_FORMAT)
#            define RZ_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) RZ_ATTR((format(__MINGW_PRINTF_FORMAT, STRING_INDEX, FIRST_TO_CHECK)))
#        else
#            define RZ_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) RZ_ATTR((format(printf, STRING_INDEX, FIRST_TO_CHECK)))
#        endif
#    endif
#    if (!defined(RZ_PRINTF_FORMAT))
#        define RZ_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#    endif
#    if (!defined(RZ_PRINTF_FMT))
#        define RZ_PRINTF_FMT(FMT) FMT
#    endif

#    if (defined(RZ_WARN_DEPRECATED) && !defined(RZ_DEPRECATED))
#        if RZ_TARGET_COMPILER_MSVC // https://en.cppreference.com/c/compiler_support/23
#            define RZ_DEPRECATED(message) __declspec(deprecated(message))
#        elif RZ_TARGET_ANY(COMPILER, GCC, CLANG)
#            define RZ_DEPRECATED(message) [[deprecated(message)]]
#        else
#            define RZ_DEPRECATED(...)
#        endif
#    else
#        define RZ_DEPRECATED(...)
#    endif
#    if defined(__cplusplus)
#        if RZ_TARGET_COMPILER_CLANG
#            define RZ__INITIALIZER_BEGIN_DISABLE_WARNINGS _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"")
#            define RZ__INITIALIZER_END_DISABLE_WARNINGS   _Pragma("clang diagnostic pop")
#        else
#            define RZ__INITIALIZER_BEGIN_DISABLE_WARNINGS
#            define RZ__INITIALIZER_END_DISABLE_WARNINGS
#        endif
#        define RZ__INITIALIZER(NAME)                                                                                                 \
            struct RZ__initializer_register_##NAME {                                                                                  \
                RZ__initializer_register_##NAME();                                                                                    \
            };                                                                                                                        \
            RZ__INITIALIZER_BEGIN_DISABLE_WARNINGS                                                                                    \
            static RZ__initializer_register_##NAME rz__initializer_register_##FIXTURE##NAME##_g RZ__INITIALIZER_END_DISABLE_WARNINGS; \
            RZ__initializer_register_##NAME::RZ__initializer_register_##NAME()

#    elif RZ_TARGET_COMPILER_MSVC
#        if RZ_TARGET_OS_WINDOWS && RZ_TARGET_BITS_64
#            define RZ__SYMBOL_PREFIX
#        else
#            define RZ__SYMBOL_PREFIX "_"
#        endif
#        if RZ_TARGET_COMPILER_CLANG
#            define RZ__INITIALIZER_BEGIN_DISABLE_WARNINGS _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wmissing-variable-declarations\"")
#            define RZ__INITIALIZER_END_DISABLE_WARNINGS   _Pragma("clang diagnostic pop")
#        else
#            define RZ__INITIALIZER_BEGIN_DISABLE_WARNINGS
#            define RZ__INITIALIZER_END_DISABLE_WARNINGS
#        endif
#        pragma section(".CRT$XCU", read)
#        define RZ__INITIALIZER(NAME)                                                                                                        \
            static void __cdecl rz__initializer_register_##NAME(void);                                                                       \
            RZ__INITIALIZER_BEGIN_DISABLE_WARNINGS                                                                                           \
            __pragma(comment(linker, "/include:" RZ__SYMBOL_PREFIX RZ_STRINGIFY(rz__initializer_register_##NAME) "_")) RZ_EXTERN_C           \
                __declspec(allocate(".CRT$XCU")) void(__cdecl * rz__initializer_register_##NAME##_)(void) = rz__initializer_register_##NAME; \
            RZ__INITIALIZER_END_DISABLE_WARNINGS                                                                                             \
            static void __cdecl rz__initializer_register_##NAME(void)
#    else
#        if RZ_TARGET_OS_LINUX
#            if RZ_TARGET_COMPILER_CLANG
#                if __has_warning("-Wreserved-id-macro")
#                    pragma clang diagnostic push
#                    pragma clang diagnostic ignored "-Wreserved-id-macro"
#                endif
#            endif
#            define __STDC_FORMAT_MACROS 1
#            if RZ_TARGET_COMPILER_CLANG
#                if __has_warning("-Wreserved-id-macro")
#                    pragma clang diagnostic pop
#                endif
#            endif
#        endif
#        define RZ__INITIALIZER(NAME)                    \
            static void NAME(void) RZ_ATTR(constructor); \
            static void NAME(void)
#    endif

#    define RZ_UNUSED(value)   (void)(value)
#    define RZ_COMMA           ,
#    define RZ_UNUSED_ALL(...) RZ_FOR_EACH(;, RZ_UNUSED, __VA_ARGS__)
#    if (defined(__cplusplus))
#        define RZ_TYPEOF(x) declspec(x)
#        define RZ_EXTERN_C  extern "C"
#    else
#        define RZ_TYPEOF(x) typeof(x)
#        define RZ_EXTERN_C
#    endif                                                                // if defined(__cplusplus)
#    define RZ_ADDRESSOF(typevar, value) ((RZ_TYPEOF(typevar)[1]){value}) // literal array decays to pointer to value
#    define RZ_OFFSETOF(var, field)      offsetof(var, field)
#    if (RZ_HAS_BUILTIN(__builtin_types_compatible_p))
#        define RZ_TYPES_COMPATIBLE(A, B) (__builtin_types_compatible_p(RZ_TYPEOF(A), RZ_TYPEOF(B)))
#    else
#        define RZ_TYPES_COMPATIBLE(A, B) (_Generic(A, RZ_TYPEOF(B): true, default: false))
#    endif

#    define RZ_STATIC_ASSERT                       static_assert
#    define RZ_STATIC_ASSERT_TYPE_COMPATIBLE(A, B) RZ_STATIC_ASSERT(RZ_TYPES_COMPATIBLE(A, B), "typeof (" RZ_STRINGIFY(A) ") and (" RZ_STRINGIFY(B) ") is not compatible")
#    define RZ_ASSERT(EXPR, ...)                   ((void)((!!(EXPR)) || (RZ_PANIC("'" #EXPR "' - " __VA_ARGS__), 0)))
#    define RZ_ASSERT_NOT_NULL(PTR)                RZ_ASSERT((PTR) != NULL && "Ptr should not be NULL")
#    define RZ_ASSERT_ALLOCATOR_PTR(PTR)           RZ_ASSERT((PTR) != NULL && "Allocator Return NULL. Memory Full?")
#    if RZ_DEBUG
#        define RZ_DBG_ASSERT(EXPR, ...) RZ_ASSERT(EXPR, "DEBUG ASSERTION: " __VA_ARGS__)
#    else
#        define RZ_DBG_ASSERT(...)
#    endif // if RZ_DEBUG
#    define RZ_DBG_ASSERT_NOT_NULL(PTR)      RZ_ASSERT((PTR) != NULL && "Ptr should not be NULL")
#    define RZ_DBG_ASSERT_ALLOCATOR_PTR(PTR) RZ_ASSERT((PTR) != NULL && "Allocator Return NULL. Memory Full?")

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

#    define rz_opt_unwrap(opt)          (RZ_ASSERT(opt.is_some),  opt.unwrap)
#    define rz_result_unwrap(result)    (RZ_ASSERT(result.is_ok), result.unwrap)

#    define rz_in_range(v, min, max)    (((v) >= (min)) && ((v) <= (max)))
#    define rz_is_eq(a, b)              (a == b)

#    define RZ_ENUM_BIT(NAME, TYPE) \
        typedef TYPE NAME;          \
        enum NAME##_enum : TYPE

/* Basic non-atomic macros (operate on integer variables) */
#    define rz_bit(bits, bit)                ((bits) & (bit))
#    define rz_bit_set(bits, bit)            ((bits) |= (bit))
#    define rz_bit_clear(bits, bit)          ((bits) &= ~(bit))
#    define rz_bit_toggle(bits, bit)         ((bits) ^= (bit))
#    define rz_bit_all(bits, ...)            RZ_ALL(rz_bit, __VA_ARGS__)
#    define rz_bit_set_all(bits, ...)        rz_bit_set(bits, RZ_FOR_EACH(|, RZ_NOP, __VA_ARGS__))
#    define rz_bit_clear_all(bits, ...)      rz_bit_clear(bits, RZ_FOR_EACH(|, RZ_NOP, __VA_ARGS__))
#    define rz_bit_toggle_all(bits, ...)     rz_bit_toggle(bits, RZ_FOR_EACH(|, RZ_NOP, __VA_ARGS__))
#    define RZ_PTR_CAST_SAME_SIZE(TYPE, PTR) ((sizeof(TYPE) == sizeof(*(PTR))) ? ((TYPE *)PTR) : size_is_not_the_same)
// clang-format on

#    ifdef __cplusplus
extern "C" {
#    endif

typedef uint8_t   rz_u8;
typedef int8_t    rz_i8;
typedef uint16_t  rz_u16;
typedef int16_t   rz_i16;
typedef uint32_t  rz_u32;
typedef int32_t   rz_i32;
typedef uint64_t  rz_u64;
typedef int64_t   rz_i64;
typedef double    rz_f64;
typedef float     rz_f32;
typedef size_t    rz_usize;
typedef ptrdiff_t rz_isize;
typedef ptrdiff_t rz_ptrdiff;
typedef uintptr_t rz_uptr;
typedef intptr_t  rz_iptr;
typedef char      rz_char;
typedef wchar_t   rz_wchar;
typedef int       rz_int;

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
#    define RZ_USIZE_MAX SIZE_MAX
#    define RZ_MAX(A, B) (((A) > (B)) ? (A) : (B))
#    define RZ_MIN(A, B) (((A) < (B)) ? (A) : (B))

// defined RZ_WINDOWS_MSGBOX_ERROR if you want panic using windows MessageBox
RZ_DEF RZ_NORETURN void rz_panic_impl(const char *loc, RZ_PRINTF_FMT(const char *fmt), ...) RZ_PRINTF_FORMAT(2, 3);

#    define RZ_ANSI_RESET             "\x1b[0m"
#    define RZ_ANSI_BOLD              "\x1b[1m"
#    define RZ_ANSI_DIM               "\x1b[2m"
#    define RZ_ANSI_UNDERLINE         "\x1b[4m"
#    define RZ_ANSI_REVERSED          "\x1b[7m"

#    define RZ_ANSI_BLACK             "\x1b[30m"
#    define RZ_ANSI_RED               "\x1b[31m"
#    define RZ_ANSI_GREEN             "\x1b[32m"
#    define RZ_ANSI_YELLOW            "\x1b[33m"
#    define RZ_ANSI_BLUE              "\x1b[34m"
#    define RZ_ANSI_MAGENTA           "\x1b[35m"
#    define RZ_ANSI_CYAN              "\x1b[36m"
#    define RZ_ANSI_WHITE             "\x1b[37m"
#    define RZ_ANSI_BRIGHT_BLACK      "\x1b[90m"
#    define RZ_ANSI_BRIGHT_RED        "\x1b[91m"
#    define RZ_ANSI_BRIGHT_GREEN      "\x1b[92m"
#    define RZ_ANSI_BRIGHT_YELLOW     "\x1b[93m"
#    define RZ_ANSI_BRIGHT_BLUE       "\x1b[94m"
#    define RZ_ANSI_BRIGHT_MAGENTA    "\x1b[95m"
#    define RZ_ANSI_BRIGHT_CYAN       "\x1b[96m"
#    define RZ_ANSI_BRIGHT_WHITE      "\x1b[97m"

#    define RZ_ANSI_BG_BLACK          "\x1b[40m"
#    define RZ_ANSI_BG_RED            "\x1b[41m"
#    define RZ_ANSI_BG_GREEN          "\x1b[42m"
#    define RZ_ANSI_BG_YELLOW         "\x1b[43m"
#    define RZ_ANSI_BG_BLUE           "\x1b[44m"
#    define RZ_ANSI_BG_MAGENTA        "\x1b[45m"
#    define RZ_ANSI_BG_CYAN           "\x1b[46m"
#    define RZ_ANSI_BG_WHITE          "\x1b[47m"
#    define RZ_ANSI_BG_BRIGHT_BLACK   "\x1b[100m"
#    define RZ_ANSI_BG_BRIGHT_RED     "\x1b[101m"
#    define RZ_ANSI_BG_BRIGHT_GREEN   "\x1b[102m"
#    define RZ_ANSI_BG_BRIGHT_YELLOW  "\x1b[103m"
#    define RZ_ANSI_BG_BRIGHT_BLUE    "\x1b[104m"
#    define RZ_ANSI_BG_BRIGHT_MAGENTA "\x1b[105m"
#    define RZ_ANSI_BG_BRIGHT_CYAN    "\x1b[106m"
#    define RZ_ANSI_BG_BRIGHT_WHITE   "\x1b[107m"

RZ_DEF bool rz_isatty(FILE *f);

#    ifdef __cplusplus
} /* extern "C" */
#    endif

#endif /* end of include guard: RZ_COMMON_H */
