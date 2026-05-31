#include "rz_error.h"
#include "rz_logger.h"
#include "rz_strings.h"

thread_local RZ_StrBuilder rz__error_static = {0};
static void                rz__strerror_impl(RZ_OsError errnum, RZ_StrBuilder *sb);

RZ_DEF const char *rz_strerror(void) {
    if (!rz_arr_is_empty(&rz__error_static)) {
        return "OK: no error";
    }
    return rz_str_to_cstr(&rz__error_static);
}

RZ_DEF void rz_set_error(RZ_OsError errnum, const char *tag, const char *file, rz_int line, RZ_PRINTF_FMT(const char *fmt), ...) RZ_PRINTF_FORMAT(2, 3) {
    if (!rz_is_allocator(rz__error_static.allocator)) {
        rz__error_static.allocator = rz_std_allocator();
        rz_arr_reserve(&rz__error_static, RZ_MAX_BUFFER_STRERROR_LEN);
    }

    // reset buffer
    rz__error_static.len = 0;

    va_list arg;
    va_start(arg, fmt);
    rz_str_appendvf(&rz__error_static, fmt, arg);
    va_end(arg);

    if (errnum != RZ_NO_ERROR) {
        rz_str_append_cstr(&rz__error_static, "(OS Error: ");
        rz__strerror_impl(errnum, &rz__error_static);
        rz_str_append_cstr(&rz__error_static, ")");
    }

    if ((RZ_LOG_LEVEL_ERROR <= RZ_LOG_MAX_LEVEL) && (RZ_LOG_LEVEL_ERROR <= rz_log_max_level())) {
        RZ_LogRecord record = {.level = RZ_LOG_LEVEL_ERROR, .tag = tag, .loc_file = file, .loc_line = line};
        rz_log(rz_log_logger(), record, "%s", rz_str_to_cstr(&rz__error_static));
    }
}

static void rz__strerror_impl(RZ_OsError errnum, RZ_StrBuilder *sb) {
#if defined(RZ_OS_WINDOWS)
    char    buf[RZ_MAX_BUFFER_STRERROR_LEN] = {0};
    HMODULE module                          = NULL;
    DWORD   flags                           = 0;
    // NTSTATUS errors may be encoded as HRESULT, which may returned from
    // GetLastError. For more information about Windows error codes, see
    // `[MS-ERREF]`: https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/0642cb2f-2075-4469-918c-4441e69c548a
    if (errnum & FACILITY_NT_BIT) {
        module = GetModuleHandle(TEXT("NTDLL.DLL"));
        if (module != NULL) {
            errnum ^= FACILITY_NT_BIT;
            flags |= FORMAT_MESSAGE_FROM_HMODULE;
        }
    }
    flags |= FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD size = FormatMessageA(flags, module, errnum, LANG_USER_DEFAULT, (LPSTR)buf, sizeof(buf), NULL);
    if (size == 0) {
        rz_str_appendf(sb, "Failed to get os_error message of (os_error: %lu). FormatMessageA() error: %lu", errnum, rz_last_os_error());
        return;
    }
    rz_str_append_sized_str(sb, buf, size);
#else
    const char *err_cstr = strerror(err);
    rz_str_append_cstr(sb, err_cstr);
#endif
}
