#include "rz_common.h"
#include "rz_sprintf.h"

RZ_DEC void rz_panic_impl(const char *loc, RZ_PRINTF_FMT(const char *fmt), ...) {

    va_list arg;
    va_start(arg);
    char *err_msg = rz_tvsprintf(fmt, arg);
    char *msg     = rz_tsprintf("%s: PANIC: %s", loc, err_msg);
    va_end(arg);

#if (defined(RZ_WINDOWS_MSGBOX_ERROR) && defined(RZ_OS_WINDOWS))
    MessageBoxA(NULL, msg, "Error Panic", MB_OK);
    ExitProcess(1);
#else
    fputs(msg, stderr);
    fputs(RZ_ENDLINE, stderr);
    abort();
#endif
}

RZ_DEC const char *rz_strerror(void) {
    static thread_local char BUFFER[MAX_BUFFER_STRERROR_LEN] = {0};
    auto                     err                             = rz_errno;
#if defined(RZ_OS_WINDOWS)
    if (err == NO_ERROR) goto no_error;
    DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK;
    if (FormatMessageA(flags, NULL, err, LANG_USER_DEFAULT, (LPSTR)BUFFER, MAX_BUFFER_STRERROR_LEN, NULL) == 0) {
        MessageBoxA(NULL, "FormatMessage failed", "Error", MB_OK);
        ExitProcess(err);
    }
    return BUFFER;
#else
    if (err == 0) goto no_error;
    strerror_s(BUFFER, MAX_BUFFER_STRERROR_LEN, err);
    return BUFFER;
#endif

no_error:
    BUFFER[0] = 0;
    return BUFFER;
}

RZ_DEC bool rz_isatty(FILE *f) {
#ifdef RZ_OS_WINDOWS
    return _isatty(_fileno(f));
#else
    return isatty(fileno(f));
#endif
}
