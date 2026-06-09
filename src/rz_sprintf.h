#pragma once

#define RZ_SPRINTF_STRING_VIEW
#ifndef RZ_SPRINTF_H
#    define RZ_SPRINTF_H

/// #BEGIN INCLUDE
#    include "rz_allocator.h"
#    include "rz_common.h"
/// #END INCLUDE

/// #BEGIN DECLARATION RZ_SPRINTF

/// define RZ_SPRINTF_NOUNALIGNED before inclusion to force rz_sprintf to always use aligned accesses
/// define RZ_SPRINTF_NOFLOAT for disabling floating point support
/// define RZ_SPRINTF_STRING_VIEW for disabling floating point support
#    ifndef RZ_SPRINTF_MIN
#        define RZ_SPRINTF_MIN 512 // how many characters per callback
#    endif
#    ifndef RZ_SPRINTF_MSVC_MODE   // used for MSVC2013 and earlier (MSVC2015 matches GCC)
#        if defined(_MSC_VER) && (_MSC_VER < 1900)
#            define RZ_SPRINTF_MSVC_MODE
#        endif
#    endif

#    define RZ_SVFmt     "%.*s"
#    define RZ_SVArg(sv) ((int)(sv).len), (sv).data

#    ifdef __cplusplus
extern "C" {
#    endif

typedef char *RZ_SprintfCallback(const char *buf, void *user, int len);

RZ_DEC int  rz_vsprintf(char *buf, char const *fmt, va_list va);
RZ_DEC int  rz_vsnprintf(char *buf, int count, char const *fmt, va_list va);
RZ_DEC int  rz_sprintf(char *buf, RZ_PRINTF_FMT(char const *fmt), ...) RZ_PRINTF_FORMAT(2, 3);
RZ_DEC int  rz_snprintf(char *buf, int count, RZ_PRINTF_FMT(char const *fmt), ...) RZ_PRINTF_FORMAT(3, 4);
RZ_DEC int  rz_vsprintfcb(RZ_SprintfCallback *callback, void *user, char *buf, char const *fmt, va_list va);
RZ_DEC void rz_sprintf_set_separators(char comma, char period);

RZ_DEC rz_char *rz_avsprintf(RZ_Allocator a, const rz_char *fmt, va_list arg);
RZ_DEC rz_char *rz_asprintf(RZ_Allocator a, RZ_PRINTF_FMT(char const *fmt), ...) RZ_PRINTF_FORMAT(2, 3);
#    define rz_tvsprintf(fmt, arg) rz_avsprintf(rz_temp_allocator(), fmt, arg)
#    define rz_tsprintf(...)       rz_asprintf(rz_temp_allocator(), __VA_ARGS__)

#    define rz_fmt(v)                   \
        _Generic(v,                     \
            bool: "%s",                 \
            rz_i8: "%hhd",              \
            rz_i16: "%hd",              \
            rz_i32: "%d",               \
            signed long: "%ld",         \
            signed long long: "%lld",   \
            rz_u8: "%hhu",              \
            rz_u16: "%hu",              \
            rz_u32: "%u",               \
            unsigned long: "%lu",       \
            unsigned long long: "%llu", \
            rz_char: "%c",              \
            float: "%g",                \
            double: "%g",               \
            long double: "%Lg",         \
            char *: "%%s",              \
            char const *: "%s",         \
            void *: "%p",               \
            const void *: "%p",         \
            RZ_StrView: "%.*s",         \
            default: "%p")

#    ifdef __cplusplus
} /* extern "C" */
#    endif
/// #END DECLARATION RZ_SPRINTF

#endif /* end of include guard: RZ_SPRINTF_H */
