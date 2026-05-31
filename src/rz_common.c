#include "rz_common.h"
#include "rz_sprintf.h"

RZ_DEC void rz_panic_impl(const char *loc, RZ_PRINTF_FMT(const char *fmt), ...) {

    va_list arg;
    va_start(arg);
    char *err_msg = rz_tvsprintf(fmt, arg);
    char *msg     = rz_tsprintf("%s: PANIC: %s", loc, err_msg);
    va_end(arg);

#if (defined(RZ_OS_WINDOWS) && defined(RZ_WINDOWS_MSGBOX_ERROR))
    MessageBoxA(NULL, msg, "Error Panic", MB_OK);
    ExitProcess(1);
#else
    fputs(msg, stderr);
    fputs(RZ_ENDLINE, stderr);
    abort();
#endif
}

RZ_DEC bool rz_isatty(FILE *f) {
#ifdef RZ_OS_WINDOWS
    return _isatty(_fileno(f));
#else
    return isatty(fileno(f));
#endif
}
