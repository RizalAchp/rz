#include "rz_process.h"

RZ_DEF rz_isize rz_nprocs(void) {
#ifdef RZ_OS_WINDOWS
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);
    return siSysInfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

RZ_DEF bool rz_proc_wait(RZ_Proc proc, rz_int *exit_status) {
    if (proc == RZ_PROC_INVALID) return false;

#ifdef RZ_OS_WINDOWS
    DWORD result = WaitForSingleObject(proc, INFINITE);

    if (result == WAIT_FAILED) {
        // log(ERROR, "could not wait on child process: %s", rz_strerror());
        return false;
    }

    if (exit_status) {
        if (!GetExitCodeProcess(proc, (LPDWORD)exit_status)) {
            // log(ERROR, "could not get process exit code: %s", rz_strerror());
            return false;
        }

        if (exit_status != 0) {
            // log(ERROR, "command exited with exit code %lu", exit_status);
            return false;
        }
    }

    CloseHandle(proc);
    return true;
#else
    for (;;) {
        int wstatus = 0;
        if (waitpid(proc, &wstatus, 0) < 0) {
            // rz_log(RZ_LOG_ERROR, "could not wait on command (pid %d): %s", proc, strerror());
            return false;
        }

        if (WIFEXITED(wstatus)) {
            if (exit_status) {
                *exit_status = WEXITSTATUS(wstatus);
                if (*exit_status != 0) {
                    // rz_log(RZ_LOG_ERROR, "command exited with exit code %d", exit_status);
                    return false;
                }
            }

            break;
        }

        if (WIFSIGNALED(wstatus)) {
            // rz_log(RZ_LOG_ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
            return false;
        }
    }
    return true;
#endif
}
RZ_DEF bool rz_procs_wait(RZ_Procs *procs) {
    rz_int return_status = 0;
    bool   result        = true;
    rz_foreach(proc, procs) {
        result &= rz_proc_wait(proc, &return_status);
    }
    return result;
}

RZ_DEF bool rz_procs_flush(RZ_Procs *procs) {
    bool result = rz_procs_wait(procs);
    procs->len  = 0;
    return result;
}

RZ_DEF RZ_ProcWaitAsyncStatus rz_proc_wait_async(RZ_Proc proc, rz_int *exit_status, rz_int timeout_ms) {
    if (proc == RZ_PROC_INVALID) return false;

#ifdef RZ_OS_WINDOWS
    DWORD result = WaitForSingleObject(proc, timeout_ms);

    if (result == WAIT_TIMEOUT) {
        return RZ_PROC_WAIT_TIMEOUT;
    }

    if (result == WAIT_FAILED) {
        // rz_log(RZ_LOG_ERROR, "could not wait on child process: %s", rz_strerror());
        return RZ_PROC_WAIT_ERR;
    }

    if (exit_status) {
        if (!GetExitCodeProcess(proc, (LPDWORD)exit_status)) {
            // rz_log(RZ_LOG_ERROR, "could not get process exit code: %s", rz_strerror());
            return RZ_PROC_WAIT_ERR;
        }

        if (exit_status != 0) {
            // rz_log(RZ_LOG_ERROR, "command exited with exit code %lu", exit_status);
            return RZ_PROC_WAIT_ERR;
        }
    }

    CloseHandle(proc);

    return RZ_PROC_WAIT_OK;
#else
    long            ns       = timeout_ms * RZ_TIME_NANOS_PER_MILLI;
    struct timespec duration = {
        .tv_sec  = ns / RZ_TIME_NANOS_PER_SEC,
        .tv_nsec = ns % RZ_TIME_NANOS_PER_SEC,
    };

    int   wstatus = 0;
    pid_t pid     = waitpid(proc, &wstatus, WNOHANG);
    if (pid < 0) {
        // rz_log(RZ_LOG_ERROR, "could not wait on command (pid %d): %s", proc, strerror(errno));
        return RZ_PROC_WAIT_ERR;
    }

    if (pid == 0) {
        nanosleep(&duration, NULL);
        return RZ_PROC_WAIT_TIMEOUT;
    }

    if (WIFEXITED(wstatus)) {
        if (exit_status) {
            *exit_status = WEXITSTATUS(wstatus);
            if (*exit_status != 0) {
                // rz_log(RZ_LOG_ERROR, "command exited with exit code %d", exit_status);
                return RZ_PROC_WAIT_ERR;
            }
        }
        return RZ_PROC_WAIT_OK;
    }

    if (WIFSIGNALED(wstatus)) {
        // rz_log(RZ_LOG_ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
        return RZ_PROC_WAIT_ERR;
    }

    nanosleep(&duration, NULL);
    return RZ_PROC_WAIT_TIMEOUT;
#endif
}

RZ_DEF RZ_ProcEnvs rz_proc_get_default_envs(RZ_Allocator allocator) {
    RZ_ProcEnvs envs = {.allocator = allocator};
#ifdef RZ_OS_WINDOWS
    LPCH envblock = GetEnvironmentStringsA();
    auto p        = envblock;
    while (*p) {
        rz_usize len = strlen(p);
        // skip entries that start with '=' (hidden/special)
        if (*p != '=') {
            rz_arr_append(&envs, p);
        }
        p += len + 1;
    }

#else
    const char **envs = NULL;
#    if defined(RZ_OS_UNIX)
#        if defined(RZ_OS_FREEBSD)
    envblock = (const char **)dlsym(RTLD_DEFAULT, "environ");
#        elif defined(RZ_OS_APPLE)
    envblock = (const char **)_NSGetEnviron();
#        else
    envblock = environ;
#        endif
    while (*envblock) {
        rz_arr_append(&envs, *envblock);
        envblock++;
    }
#    endif
#    error "unknown os"
#endif

    return envs;
}

RZ_DEF void rz_proc_envs_free(RZ_ProcEnvs *envs) {
    if (rz_arr_is_empty(envs)) return;
#ifdef RZ_OS_WINDOWS
    FreeEnvironmentStringsA((LPCH)*envs->data);
#endif
    rz_arr_free(envs);
}

RZ_DEC void rz_proc_display_command(RZ_ProcCommand cmd, RZ_StrBuilder *sb) {
    for (size_t i = 0; i < cmd.len; ++i) {
        const char *arg = cmd.data[i];
        if (arg == NULL) break;
        if (i > 0) rz_str_append_cstr(sb, " ");
        if (!strchr(arg, ' ')) {
            rz_str_append_cstr(sb, arg);
        } else {
            rz_str_append(sb, '\'');
            rz_str_append_cstr(sb, arg);
            rz_str_append(sb, '\'');
        }
    }
}
