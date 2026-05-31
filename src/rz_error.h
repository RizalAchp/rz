#pragma once

#ifndef __RZ_ERROR_H
#    define __RZ_ERROR_H
#    include "rz_common.h"

#    ifdef __cplusplus
extern "C" {
#    endif

#    if defined(RZ_OS_WINDOWS)
#        define rz_last_os_error GetLastError
#        define RZ_OsError       DWORD
#        define RZ_NO_ERROR      NO_ERROR
#    else
#        define rz_last_os_error() errno
#        define RZ_OsError         rz_int
#        define RZ_NO_ERROR        0
#    endif
#    ifndef RZ_MAX_BUFFER_STRERROR_LEN
#        define RZ_MAX_BUFFER_STRERROR_LEN (2u * 1024u)
#    endif

RZ_DEC const char *rz_strerror(void);
RZ_DEC void        rz_set_error(RZ_OsError errnum, const char *tag, const char *file, rz_int line, RZ_PRINTF_FMT(const char *fmt), ...) RZ_PRINTF_FORMAT(3, 4);

#    ifdef __cplusplus
} /* extern "C" */
#    endif

#    define RZ_ERROR(TAG, MSG, ...)    rz_set_error(RZ_NO_ERROR, TAG, __FILE__, __LINE__, MSG __VA_OPT__(, ) __VA_ARGS__);
#    define RZ_OS_ERROR(TAG, MSG, ...) rz_set_error(rz_last_os_error(), TAG, __FILE__, __LINE__, MSG __VA_OPT__(, ) __VA_ARGS__);

#endif /* end of include guard: __RZ_ERROR_H */
