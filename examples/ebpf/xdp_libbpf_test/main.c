/*
 * @Author: CALM.WU
 * @Date: 2022-02-11 10:36:28
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-07-08 15:12:12
 */
#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/consts.h"
#include "utils/x_ebpf.h"

#include <bpf/libbpf.h>
#include <linux/if_link.h>
#include "xm_xdp_pass.skel.h"

// bin/xdp_libbpf_test --itf=ens160 -v

enum xdp_action_type {
    XDP_ACTION_ATTACH = 0,
    XDP_ACTION_DETACH,
};

struct args {
    int32_t itf_index;
    enum xdp_action_type action_type;
    uint32_t xdp_flags;
    bool verbose;
} env = {
    .itf_index = -1, // 所有的网卡
    .xdp_flags = XDP_FLAGS_UPDATE_IF_NOEXIST,
    .verbose = true,
};

static uint32_t __prog_id;
static sig_atomic_t __sig_exit = 0;
static const char *__optstr = "i:sfv";

static const struct option __opts[] = {
    { "itf", required_argument, NULL, 'i' },
    { "type", required_argument, NULL, 't' },
    { "skb", no_argument, NULL, 's' },
    { "force_load", no_argument, NULL, 'f' },
    { "verbose", no_argument, NULL, 'v' },
    { 0, 0, 0, 0 },
};

static void usage(const char *prog) {
    fprintf(stderr,
            "usage: %s [OPTS] --itf=IFNAME\n\n"
            "OPTS:\n"
            "    -s    use skb-mode\n"
            "    -f    force loading prog\n"
            "    -v    verbose\n",
            prog);
}

static void sig_handler(int sig) {
    __sig_exit = 1;
    warn("SIGINT/SIGTERM received, exiting...\n");

    // if (bpf_get_link_xdp_id(env.itf_index, &curr_prog_id, env.xdp_flags)) {
    //     debug("bpf_get_link_xdp_id failed\n");
    //     exit(1);
    // }
    // if (__prog_id == curr_prog_id)
    //     bpf_set_link_xdp_fd(env.itf_index, -1, env.xdp_flags);
    // else if (!curr_prog_id)
    //     debug("couldn't find a prog id on a given interface\n");
    // else
    //     debug("program on interface changed, not removing\n");

    // debug("xdp detach");
    return;
}

static void poll_stats(int32_t map_fd, int32_t xdp_stats_map_fd,
                       int32_t interval) {
    uint32_t nr_cpus = xm_bpf_num_possible_cpus();
    int32_t ret = 0;
    debug("nr_cpus: %u", nr_cpus);

    BPF_DECLARE_PERCPU(uint64_t, values);
    // uint64_t values[nr_cpus], prev[UINT8_MAX] = { 0 };
    uint64_t prev[UINT8_MAX] = { 0 };

    while (!__sig_exit) {
        xm_bpf_xdp_stats_print(xdp_stats_map_fd);

        // int32_t lookup_key = -1, next_key;
        uint32_t key = UINT32_MAX;

        // 轮询 map 中所有元素
        while (bpf_map_get_next_key(map_fd, &key, &key) == 0) {
            if (__sig_exit)
                break;

            uint64_t sum = 0;
            // TODO:
            // values 是个 core 核数的数组，这是 BPF_MAP_TYPE_PERCPU_ARRAY
            // 特性？
            ret = bpf_map_lookup_elem(map_fd, &key, values);
            if (0 == ret) {
                for (uint32_t i = 0; i < nr_cpus; i++) {
                    // sum += values[i];
                    sum += bpf_percpu(values, i);
                }
                // debug("proto: %d, lookup_key: %d sum: %lu, prev_sum: %lu\n",
                // next_key, lookup_key, sum,
                //       prev[next_key]);
                if (sum > prev[key]) {
                    debug("proto %d: sum rx_cnt: %lu, %lu pkt/s", key, sum,
                          (sum - prev[key]) / interval);
                }
                prev[key] = sum;
            } else {
                error("ERR: bpf_map_lookup_elem failed key:0x%X (ret:%d): %s",
                      key, ret, strerror(-ret));
            }
            // lookup_key = next_key;
        }

        sleep(interval);
    }
    return;
}

int32_t main(int32_t argc, char **argv) {
    int32_t ret = 0;
    int32_t opt;

    while ((opt = getopt_long(argc, argv, __optstr, __opts, NULL)) != -1) {
        switch (opt) {
        case 'v':
            env.verbose = true;
            break;
        case 'i':
            env.itf_index = if_nametoindex(optarg);
            if (unlikely(!env.itf_index)) {
                fprintf(stderr, "invalid interface name: %s, err: %s\n", optarg,
                        strerror(errno));
                exit(EXIT_FAILURE);
            }
            fprintf(stdout, "ift_index:%d, itf_name:%s", env.itf_index, optarg);
            break;
        case 's':
            env.xdp_flags |= XDP_FLAGS_SKB_MODE;
            fprintf(stdout, "xdp flags set skb mode");
            break;
        case 'f':
            env.xdp_flags &= ~XDP_FLAGS_UPDATE_IF_NOEXIST;
            fprintf(stdout, "xdp flags force load");
            break;
        case 't':
            if (!strcmp(optarg, "attach")) {
                env.action_type = XDP_ACTION_ATTACH;
            } else if (!strcmp(optarg, "detach")) {
                env.action_type = XDP_ACTION_DETACH;
            } else {
                fprintf(stderr, "invalid action type: %s\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;
        default:
            usage(basename(argv[0]));
            return 1;
        }
    }

    if (!(env.xdp_flags & XDP_FLAGS_SKB_MODE)) {
        env.xdp_flags |= XDP_FLAGS_DRV_MODE;
    }

    // if (optind == argc) {
    //     debug("optind:%d, argc:%d", optind, argc);
    //     usage(basename(argv[0]));
    //     return 1;
    // }

    libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
    // Libbpf 日志
    if (env.verbose) {
        libbpf_set_print(xm_bpf_printf);
    }

    ret = bump_memlock_rlimit();
    if (ret) {
        fprintf(stderr, "failed to increase memlock rlimit: %s\n",
                strerror(errno));
        return -1;
    }

    if (log_init("../../log.cfg", "xdp_libbpf_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    // 打开
    struct xm_xdp_pass_bpf *obj = xm_xdp_pass_bpf__open();
    if (unlikely(!obj)) {
        fprintf(stderr, "failed to open and/or load BPF object\n");
        return -1;
    } else {
        debug("BPF object opened");
    }

    // 设置初始值
    memset(obj->rodata->target_name, 0, sizeof(obj->rodata->target_name));
    strcpy(obj->rodata->target_name, "xdp_libbpf");

    // 加载
    ret = xm_xdp_pass_bpf__load(obj);
    if (unlikely(0 != ret)) {
        fprintf(stderr, "failed to load BPF object: %d\n", ret);
        goto cleanup;
    } else {
        debug("BPF object loaded");
    }

    // debug("xdp_prog_simple name: %s sec_name: %s",
    // obj->progs.xdp_prog_simple->name,
    //       obj->progs.xdp_prog_simple->sec_name);

    struct bpf_map_info map_info;

    int32_t ipproto_rx_cnt_map_fd = bpf_map__fd(obj->maps.ipproto_rx_cnt_map);
    xm_bpf_get_bpf_map_info(ipproto_rx_cnt_map_fd, &map_info, 1);
    debug("ipproto_rx_cnt map fd:%d, id: %d", ipproto_rx_cnt_map_fd,
          map_info.id);

    int32_t xdp_stats_map_fd = bpf_map__fd(obj->maps.xdp_stats_map);
    xm_bpf_get_bpf_map_info(xdp_stats_map_fd, &map_info, 1);
    debug("xdp_stats_map fd:%d, id:%d", xdp_stats_map_fd, map_info.id);

    int32_t prog_fd = bpf_program__fd(obj->progs.xdp_prog_simple);
    debug("bpf prog xdp_prog_simple fd:%d", prog_fd);

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // 附加 它是可选的，你可以通过直接使用 libbpf API 获得更多控制）；
    // ret = xdp_pass_bpf__attach(obj);
    // ret = bpf_xdp_attach(prog_fd, env.itf_index, __xdp_flags, NULL);
    // 使用 bpf_set_link_xdp_fd 执行 attach 成功，和 bpf_xdp_attach 差异在于
    // old_prog_fd 这个参数
    ret = xm_bpf_xdp_link_attach(env.itf_index, env.xdp_flags, prog_fd);
    // ret = bpf_set_link_xdp_fd(env.itf_index, prog_fd, env.xdp_flags);
    if (ret < 0) {
        fprintf(stderr, "link set xdp fd failed. ret: %d err: %s\n", ret,
                strerror(errno));
        goto cleanup;
    } else {
        debug("BPF programs attached");
    }

    struct bpf_prog_info info = { 0 };
    uint32_t info_len = sizeof(info);

    ret = bpf_obj_get_info_by_fd(prog_fd, &info, &info_len);
    if (ret) {
        fprintf(stderr, "can't get prog info: %s\n", strerror(errno));
    }
    __prog_id = info.id;
    debug("prog id: %d", __prog_id);

    poll_stats(ipproto_rx_cnt_map_fd, xdp_stats_map_fd, 2);

    // bpf_xdp_detach(env.itf_index, __xdp_flags, NULL);

cleanup:
    xm_bpf_xdp_link_detach(env.itf_index, env.xdp_flags, __prog_id);

    xm_xdp_pass_bpf__destroy(obj);

    debug("%s exit\n", argv[0]);

    log_fini();
    return 0;
}