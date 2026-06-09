#include "rz_common.h"
#include "rz_sprintf.h"

RZ_DEC void rz_panic_impl(const char *loc, RZ_PRINTF_FMT(const char *fmt), ...) {

    va_list arg;
    va_start(arg);
    char *err_msg = rz_tvsprintf(fmt, arg);
    char *msg     = rz_tsprintf("%s: PANIC: %s", loc, err_msg);
    va_end(arg);

#if RZ_TARGET_OS_WINDOWS && defined(RZ_WINDOWS_MSGBOX_ERROR)
    MessageBoxA(NULL, msg, "Error Panic", MB_OK);
    ExitProcess(1);
#else
    fputs(msg, stderr);
    fputs(RZ_ENDLINE, stderr);
    abort();
#endif
}

RZ_DEC bool rz_isatty(FILE *f) {
#if RZ_TARGET_OS_WINDOWS
    DWORD  mode;
    HANDLE h = (HANDLE)_get_osfhandle(_fileno(ctx->output));
    if (h != INVALID_HANDLE_VALUE) {
        if (GetConsoleMode(h, &mode)) {
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            return true;
            // Now ANSI sequences like \x1b[31m will work
        }
    }
    return false;
#else
    return isatty(fileno(f));
#endif
}
