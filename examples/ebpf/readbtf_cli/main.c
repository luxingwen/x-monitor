/*
 * @Author: CALM.WU
 * @Date: 2022-06-28 14:19:24
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-25 10:15:48
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/consts.h"
#include "utils/strings.h"
#include "utils/x_ebpf.h"

#include <bpf/btf.h>
#include <bpf/libbpf.h>

static struct args {
    bool verbose;
    bool find_in_module;
} __env = {
    .verbose = true,
    .find_in_module = false,
};

static const struct option __opts[] = {
    { "verbose", no_argument, NULL, 'v' },
    { "module", optional_argument, NULL, 'm' },
    { "symbol", required_argument, NULL, 's' },
    { 0, 0, 0, 0 },
};

static const char *__optstr = "vm:s:";

static void usage(const char *prog) {
    fprintf(stderr,
            "usage: %s [OPTS]\n\n"
            "OPTS:\n"
            "    -m    module\n"
            "    -s    symbol\n"
            "    -v    verbose\n",
            prog);
}

static void btf_dump_printf(void *ctx, const char *fmt, va_list args) {
    vfprintf(stdout, fmt, args);
}

int32_t main(int32_t argc, char **argv) {
    int32_t err = 0;
    int32_t opt = 0;
    int32_t type_id = 0;
    struct btf_dump *dump = NULL;
    struct btf *vmlinux_btf = NULL, *module_btf = NULL, *target_btf = NULL;
    char *module_name = NULL, *symbol_name = NULL;

    while ((opt = getopt_long(argc, argv, __optstr, __opts, NULL)) != -1) {
        switch (opt) {
        case 'v':
            __env.verbose = true;
            break;
        case 'm':
            __env.find_in_module = true;
            module_name = strndup(optarg, strlen(optarg));
            break;
        case 's':
            symbol_name = strndup(optarg, strlen(optarg));
            break;
        default:
            fprintf(stderr, "Unrecognized option '%c'\n", opt);
            usage(basename(argv[0]));
            return EXIT_FAILURE;
        }
    }

    if (unlikely(!symbol_name)) {
        usage(basename(argv[0]));
        return EXIT_FAILURE;
    }

    if (log_init("../examples/log.cfg", "readbtf_cli") != 0) {
        fprintf(stderr, "log init failed\n");
        return EXIT_FAILURE;
    }

    libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
    if (__env.verbose) {
        libbpf_set_print(xm_bpf_printf);
    }

    // 从vmlinux中读取BTF信息
    vmlinux_btf = btf__load_vmlinux_btf();
    err = libbpf_get_error(vmlinux_btf);
    if (unlikely(0 != err)) {
        error("btf__load_vmlinux_btf() failed! ERROR(%d)", err);
        goto cleanup;
    }
    target_btf = vmlinux_btf;

    if (__env.find_in_module) {
        // 从指定的模块中读取BTF信息
        module_btf = btf__load_module_btf(module_name, vmlinux_btf);
        err = libbpf_get_error(module_btf);
        if (unlikely(0 != err)) {
            error("btf__load_module_btf() failed! ERROR(%d)", err);
            goto cleanup;
        }
        target_btf = module_btf;
    }

    type_id = btf__find_by_name(target_btf, symbol_name);
    if (unlikely(type_id < 0)) {
        err = type_id;
        error("btf__load_vmlinux_btf() symbol_name: '%s' failed! ERROR(%d)",
              symbol_name, err);
        goto cleanup;
    }

    info("Module: '%s' Symbol: %s have BTF id:%d", module_name, symbol_name,
         type_id);

    dump = btf_dump__new(target_btf, NULL, NULL, btf_dump_printf);

    btf_dump__dump_type(dump, type_id);

cleanup:
    if (likely(module_name)) {
        free(module_name);
    }

    if (likely(symbol_name)) {
        free(symbol_name);
    }

    if (likely(NULL != module_btf)) {
        btf__free(module_btf);
    }

    if (likely(NULL != vmlinux_btf)) {
        btf__free(vmlinux_btf);
    }

    if (likely(dump)) {
        btf_dump__free(dump);
    }

    log_fini();

    if (err)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}