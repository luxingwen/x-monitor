/*
 * @Author: CALM.WU
 * @Date: 2022-09-13 11:49:45
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-13 15:21:40
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/consts.h"
#include "utils/x_ebpf.h"
#include "utils/strings.h"

#include <bpf/libbpf.h>
#include "bpf_and_user.h"
#include "xm_deny_ptrace.skel.h"

static struct main_args {
    bool  verbose;
    pid_t ptrace_pid;
} __args = {
    .verbose = false,
    .ptrace_pid = 0,
};

const char *argp_program_version = "deny_ptrace 0.1";
const char *argp_program_bug_address = "<wubo0067@hotmail.com>";
const char  argp_program_doc[] = "deny_ptrace -- deny ptrace by ebpf\n"
                                 "\n"
                                 "USAGE: deny_ptrace [-t pid] -v \n";

static struct argp_option __options[] = {
    { "verbose", 'v', 0, 0, "Verbose debug info", 0 },
    { "pid", 't', "PID", 0, "The pid to be traced", 0 },
    { 0 },
};

static sig_atomic_t __sig_exit = 0;

static void __sig_handler(int UNUSED(sig)) {
    __sig_exit = 1;
    warn("SIGINT/SIGTERM received, exiting...\n");
}

static error_t __parse_arg(int32_t key, char *arg, struct argp_state *state) {
    switch (key) {
    case 't':
        __args.ptrace_pid = str2int32_t(arg);
        if (unlikely(__args.ptrace_pid < 0)) {
            error("invalid pid: %s", arg);
            argp_usage(state);
        }
        break;
    case 'v':
        __args.verbose = true;
        break;
    case ARGP_KEY_ARG:
        argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static const struct argp __argp = {
    .options = __options,
    .parser = __parse_arg,
    .doc = argp_program_doc,
};

static int32_t __handle_event(void *UNUSED(ctx), void *data, size_t UNUSED(data_sz)) {
    const struct xm_ebpf_event *evt = (const struct xm_ebpf_event *)data;
    if (0 == evt->err_code) {
        debug("Killed PID:%d (%s) for trying to use ptrace to attach pid:%d", evt->pid, evt->comm,
              __args.ptrace_pid);
    } else {
        error("Failed t kill PID:%d (%s) for trying to use ptrace to attach pid:%d", evt->pid,
              evt->comm, __args.ptrace_pid);
    }
    return 0;
}

int32_t main(int32_t argc, char **argv) {
    int32_t                    ret = 0;
    struct xm_deny_ptrace_bpf *skel = NULL;
    struct ring_buffer        *dp_evt_rb = NULL;

    // init log
    if (log_init("../examples/log.cfg", "deny_ptrace_cli") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    ret = argp_parse(&__argp, argc, argv, 0, NULL, NULL);
    if (unlikely(0 != ret)) {
        goto cleanup;
    }

    libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
    if (__args.verbose) {
        libbpf_set_print(xm_bpf_printf);
    }

    ret = bump_memlock_rlimit();
    if (ret) {
        fatal("failed to increase memlock rlimit: %s\n", strerror(errno));
        return -1;
    }

    // Open BPF application
    skel = xm_deny_ptrace_bpf__open();
    if (unlikely(!skel)) {
        error("Failed to load and verify BPF skeleton");
        goto cleanup;
    }

    // Set the ID of the process rejected by ptrace
    if (0 == __args.ptrace_pid) {
        __args.ptrace_pid = getpid();
    }

    skel->rodata->target_pid = __args.ptrace_pid;
    debug("set deny ptrace pid:'%d'", __args.ptrace_pid);

    // Verify and load ebpf program
    ret = xm_deny_ptrace_bpf__load(skel);
    if (unlikely(0 != ret)) {
        error("Failed to load and verify BPF skeleton. err:'%s'", strerror(errno));
        goto cleanup;
    }

    // Attach tracepoint handler
    ret = xm_deny_ptrace_bpf__attach(skel);
    if (unlikely(0 != ret)) {
        error("Failed to attach BPF program. err:'%s'", strerror(errno));
        goto cleanup;
    }

    // set up ring buffer
    dp_evt_rb = ring_buffer__new(bpf_map__fd(skel->maps.xm_dp_evt_rb), __handle_event, NULL, NULL);
    if (unlikely(!dp_evt_rb)) {
        error("Failed to create ring buffer. err:'%s'", strerror(errno));
        goto cleanup;
    }

    signal(SIGINT, __sig_handler);
    signal(SIGTERM, __sig_handler);

    debug("deny_ptrace successfully started.");

    while (!__sig_exit) {
        ret = ring_buffer__poll(dp_evt_rb, 100 /* timeout, ms */);
        /* Ctrl-C will cause -EINTR */
        if (-EINTR == ret) {
            break;
        }

        if (ret < 0) {
            error("Failed to poll ring buffer. err:'%s'", strerror(errno));
            break;
        }
    }

cleanup:
    if (dp_evt_rb) {
        ring_buffer__free(dp_evt_rb);
    }

    if (skel) {
        xm_deny_ptrace_bpf__destroy(skel);
    }

    debug("deny_ptrace say byebye!");
    log_fini();
    return 0;
}