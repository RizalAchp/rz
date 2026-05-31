#pragma once

#ifndef __RZ_PROCESS_H
#    define __RZ_PROCESS_H
#    include "rz_common.h"
#    include "rz_fs.h"
#    include "rz_strings.h"

#    ifdef RZ_OS_WINDOWS
#        define RZ_Proc         HANDLE
#        define RZ_PROC_INVALID INVALID_HANDLE_VALUE
#    else
#        define RZ_Proc         int
#        define RZ_PROC_INVALID -1
#    endif

RZ_EXTERN_C_BEGIN

RZ_DEC rz_isize rz_nprocs(void);

typedef RZ_Array(RZ_Proc) RZ_Procs;

RZ_DEC bool rz_proc_wait(RZ_Proc proc, rz_int *exit_status);
RZ_DEC bool rz_procs_wait(RZ_Procs *procs);
RZ_DEC bool rz_procs_flush(RZ_Procs *procs);

typedef enum : rz_i8
{
    RZ_PROC_WAIT_ERR = -1,
    RZ_PROC_WAIT_OK,
    RZ_PROC_WAIT_TIMEOUT,
} RZ_ProcWaitAsyncStatus;
RZ_DEC RZ_ProcWaitAsyncStatus rz_proc_wait_async(RZ_Proc proc, rz_int *exit_status, rz_int timeout_ms);

typedef RZ_Array(const rz_char *) RZ_ProcCommand;
typedef RZ_Array(const rz_char *) RZ_ProcEnvs;

RZ_DEC RZ_ProcEnvs rz_proc_get_default_envs(RZ_Allocator allocator);
RZ_DEC void        rz_proc_envs_free(RZ_ProcEnvs *envs);

RZ_DEC void rz_proc_display_command(RZ_ProcCommand cmd, RZ_StrBuilder *sb);
#    define rz_proc_cmd_arg(cmd, arg)  rz_arr_append(cmd, arg)
#    define rz_proc_cmd_args(cmd, ...) rz_arr_append_many(cmd, (const char **){__VA_ARGS__}, sizeof({__VA_ARGS__}) / sizeof(const char **))
#    define rz_proc_cmd_reset(cmd)     (cmd)->len = 0

typedef struct {
    // Do not reset the command after execution.
    bool no_reset;
    // Run the command asynchronously appending its Nob_Proc to the provided Nob_Procs array
    RZ_Procs *async;
    // Maximum processes allowed in the .async list. Zero implies nob_nprocs().
    rz_usize async_limit;
    // [optional] env to pass to the command execution
    RZ_ProcEnvs envs;

    RZ_Fd *fd_in;
    RZ_Fd *fd_out;
    RZ_Fd *fd_err;
} RZ_ProcRunOpt;

RZ_DEC bool rz_proc_run_opt(RZ_ProcCommand *cmd, RZ_ProcRunOpt opt);
#    define rz_proc_run(cmd, ...) rz_proc_run_opt(cmd, (RZ_ProcRunOpt){__VA_ARGS__})

RZ_DEC RZ_Proc          rz_proc_id(void);
RZ_DEC RZ_NORETURN void rz_proc_exit(rz_i32 code);
RZ_DEC RZ_NORETURN void rz_proc_abort(void);

RZ_EXTERN_C_END

#endif /* end of include guard: __RZ_PROCESS_H */
