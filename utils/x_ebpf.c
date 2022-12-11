/*
 * @Author: CALM.WU
 * @Date: 2021-11-03 11:37:51
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-02-19 22:44:28
 */

#include "x_ebpf.h"
#include "common.h"
#include "log.h"
#include "compiler.h"

#include <linux/bpf.h>
#include <linux/if_link.h>

#include "../plugin_ebpf/bpf_and_user.h"

#define DEBUGFS "/sys/kernel/debug/tracing/"
#define MAX_SYMS 300000
static struct ksym __syms[MAX_SYMS];
static int32_t __sym_cnt;
static const char *__ksym_empty_name = "";

static const char *__xdp_action_names[XDP_ACTION_MAX] = {

    [XDP_ABORTED] = "XDP_ABORTED",   [XDP_DROP] = "XDP_DROP",
    [XDP_PASS] = "XDP_PASS",         [XDP_TX] = "XDP_TX",
    [XDP_REDIRECT] = "XDP_REDIRECT", [XDP_UNKNOWN] = "XDP_UNKNOWN",
};

/**
 * It's a wrapper for vfprintf that prepends the level of the message to the
 * message itself
 *
 * @param level The level of the message.
 * @param fmt The format string.
 * @param args the arguments to be passed to the function
 *
 * @return The number of characters printed.
 */
int32_t xm_bpf_printf(enum libbpf_print_level level, const char *fmt,
                      va_list args) {
    // if ( level == LIBBPF_DEBUG && !g_env.verbose ) {
    // 	return 0;
    // }
    char out_fmt[128] = { 0 };
    sprintf(out_fmt, "level:{%d} %s", level, fmt);
    // vfprintf适合参数可变列表传递
    return vfprintf(stderr, out_fmt, args);
}

static __always_inline int32_t __ksym_cmp(const void *p1, const void *p2) {
    return ((const struct ksym *)p1)->addr - ((const struct ksym *)p2)->addr;
}

int32_t xm_load_kallsyms() {
    FILE *f = fopen("/proc/kallsyms", "r");
    char func[256], buf[256];
    char symbol;
    void *addr;
    int32_t i = 0;

    if (!f)
        return -ENOENT;

    while (fgets(buf, sizeof(buf), f)) {
        if (sscanf(buf, "%p %c %s", &addr, &symbol, func) != 3)
            break;
        if (!addr)
            continue;
        __syms[i].addr = (long)addr;
        __syms[i].name = strdup(func);
        i++;
    }
    fclose(f);
    __sym_cnt = i;
    qsort(__syms, __sym_cnt, sizeof(struct ksym), __ksym_cmp);
    return 0;
}

struct ksym *xm_ksym_search(long key) {
    int32_t start = 0, end = __sym_cnt;
    int32_t result;

    /* kallsyms not loaded. return NULL */
    if (__sym_cnt <= 0)
        return NULL;

    while (start < end) {
        size_t mid = start + (end - start) / 2;

        result = key - __syms[mid].addr;
        if (result < 0)
            end = mid;
        else if (result > 0)
            start = mid + 1;
        else
            return &__syms[mid];
    }

    if (start >= 1 && __syms[start - 1].addr < key && key < __syms[start].addr)
        /* valid ksym */
        return &__syms[start - 1];

    /* out of range. return _stext */
    return &__syms[0];
}

long xm_ksym_get_addr(const char *name) {
    int32_t i;

    for (i = 0; i < __sym_cnt; i++) {
        if (strcmp(__syms[i].name, name) == 0)
            return __syms[i].addr;
    }

    return 0;
}

/* open kallsyms and find addresses on the fly, faster than load + search. */
extern int32_t xm_kallsyms_find(const char *sym, unsigned long long *addr) {
    char type, name[500];
    unsigned long long value;
    int err = 0;
    FILE *f;

    f = fopen("/proc/kallsyms", "r");
    if (!f)
        return -EINVAL;

    while (fscanf(f, "%llx %c %499s%*[^\n]\n", &value, &type, name) > 0) {
        if (strcmp(name, sym) == 0) {
            *addr = value;
            goto out;
        }
    }
    err = -ENOENT;

out:
    fclose(f);
    return err;
}

const char *xm_bpf_get_ksym_name(uint64_t addr) {
    struct ksym *sym;

    if (addr == 0)
        return __ksym_empty_name;

    sym = xm_ksym_search(addr);
    if (!sym)
        return __ksym_empty_name;

    return sym->name;
}

int32_t open_raw_sock(const char *iface) {
    struct sockaddr_ll sll;
    int32_t sock;

    sock = socket(PF_PACKET, SOCK_RAW | SOCK_NONBLOCK | SOCK_CLOEXEC,
                  htons(ETH_P_ALL));
    if (sock < 0) {
        error("socket() create raw socket failed: %s", strerror(errno));
        return -errno;
    }

    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_nametoindex(iface);
    sll.sll_protocol = htons(ETH_P_ALL);
    if (bind(sock, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        error("bind() to '%s' failed: %s", iface, strerror(errno));
        close(sock);
        return -errno;
    }
    return sock;
}

const char *xm_bpf_xdpaction2str(uint32_t action) {
    if (action < XDP_ACTION_MAX)
        return __xdp_action_names[action];
    return NULL;
}

int32_t xm_bpf_get_bpf_map_info(int32_t map_fd, struct bpf_map_info *info,
                                int32_t verbose) {
    uint32_t info_len = (uint32_t)sizeof(*info);

    if (unlikely(map_fd < 0)) {
        error("invalid map fd\n");
        return -1;
    }

    int32_t ret = bpf_obj_get_info_by_fd(map_fd, info, &info_len);
    if (unlikely(0 != ret)) {
        error("bpf_obj_get_info_by_fd() failed: %s", strerror(errno));
        return -1;
    }

    if (verbose) {
        debug(" - BPF map (bpf_map_type:%d) id:%d name:%s"
              " key_size:%d value_size:%d max_entries:%d",
              info->type, info->id, info->name, info->key_size,
              info->value_size, info->max_entries);
    }
    return 0;
}

static void xdp_stats_map_get_value_array(int32_t xdp_stats_map_fd,
                                          uint32_t key,
                                          struct xdp_stats_datarec *value) {
    if (unlikely(bpf_map_lookup_elem(xdp_stats_map_fd, &key, value))) {
        error("ERR: bpf_map_lookup_elem failed key:0x%X", key);
    }
}

static void
xdp_stats_map_get_value_percpu_array(int32_t xdp_stats_map_fd, uint32_t key,
                                     struct xdp_stats_datarec *value) {
    int32_t nr_cpus = xm_bpf_num_possible_cpus();
    int32_t i;

    struct xdp_stats_datarec values[nr_cpus];

    uint64_t sum_bytes = 0;
    uint64_t sum_pkts = 0;

    if (unlikely(bpf_map_lookup_elem(xdp_stats_map_fd, &key, values)) != 0) {
        error("ERR: bpf_map_lookup_elem failed key:0x%X", key);
        return;
    }

    // 要汇总所有cpu的值
    for (i = 0; i < nr_cpus; i++) {
        sum_bytes += values[i].rx_bytes;
        sum_pkts += values[i].rx_packets;
    }
    value->rx_bytes = sum_bytes;
    value->rx_packets = sum_pkts;
}

void xm_bpf_xdp_stats_print(int32_t xdp_stats_map_fd) {
    struct bpf_map_info info = { 0 };
    if (unlikely(0 != xm_bpf_get_bpf_map_info(xdp_stats_map_fd, &info, 0))) {
        error("ERR: map via FD not compatible");
        return;
    }

    uint32_t key;

    struct xdp_stats_datarec records[XDP_ACTION_MAX];

    for (key = 0; key < XDP_ACTION_MAX; key++) {
        switch (info.type) {
        case BPF_MAP_TYPE_PERCPU_ARRAY:
            xdp_stats_map_get_value_percpu_array(xdp_stats_map_fd, key,
                                                 &records[key]);
            break;
        case BPF_MAP_TYPE_ARRAY:
            xdp_stats_map_get_value_array(xdp_stats_map_fd, key, &records[key]);
            break;
        default:
            error("ERR: map type %d not supported", info.type);
            return;
        }
    }

    for (key = 0; key < XDP_ACTION_MAX; key++) {
        if (records[key].rx_bytes == 0 && records[key].rx_packets == 0)
            continue;
        debug("%s: %lu packets, %lu bytes", __xdp_action_names[key],
              (uint64_t)records[key].rx_packets,
              (uint64_t)records[key].rx_bytes);
    }

    return;
}

int32_t xm_bpf_xdp_link_attach(int32_t ifindex, uint32_t xdp_flags,
                               int32_t prog_fd) {
    int32_t ret;

    // ret = bpf_set_link_xdp_fd(ifindex, prog_fd, xdp_flags);
    ret = bpf_xdp_attach(ifindex, prog_fd, xdp_flags, NULL);
    if (ret == -EEXIST && !(xdp_flags & XDP_FLAGS_UPDATE_IF_NOEXIST)) {
        /* Force mode didn't work, probably because a program of the
         * opposite type is loaded. Let's unload that and try loading
         * again.
         */
        uint32_t old_flags = xdp_flags;
        // 将所有mode清空
        xdp_flags &= ~XDP_FLAGS_MODES;
        xdp_flags |= (old_flags & XDP_FLAGS_SKB_MODE) ? XDP_FLAGS_DRV_MODE :
                                                        XDP_FLAGS_SKB_MODE;
        // ret = bpf_set_link_xdp_fd(ifindex, -1, xdp_flags);
        ret = bpf_xdp_attach(ifindex, -1, xdp_flags, NULL);
        if (!ret) {
            // ret = bpf_set_link_xdp_fd(ifindex, prog_fd, old_flags);
            ret = bpf_xdp_attach(ifindex, prog_fd, old_flags, NULL);
        }
    }

    if (ret < 0) {
        error("ERR: ifindex(%d) link set xdp fd failed (%d): %s", ifindex, -ret,
              strerror(-ret));

        switch (-ret) {
        case EBUSY:
        case EEXIST:
            error("Hint: XDP already loaded on device, use --force to "
                  "swap/replace");
            break;
        case EOPNOTSUPP:
            error("Hint: Native-XDP not supported, use skb-mode");
            break;
        default:
            break;
        }
    }

    return ret;
}

int32_t xm_bpf_xdp_link_detach(int32_t ifindex, uint32_t xdp_flags,
                               uint32_t expected_prog_id) {
    uint32_t curr_prog_id = 0;
    int32_t ret;

    ret = bpf_xdp_query_id(ifindex, xdp_flags, &curr_prog_id);
    if (unlikely(ret)) {
        error("ERR: get link xdp id failed (err=%d): %s", -ret, strerror(-ret));
        return ret;
    }

    if (unlikely(!curr_prog_id)) {
        info("INFO: no curr XDP prog on ifindex:%d", ifindex);
        return 0;
    }

    if (expected_prog_id && curr_prog_id != expected_prog_id) {
        error("ERR: expected prog id:(%d) no match:(%d), not removing",
              expected_prog_id, curr_prog_id);
        return -1;
    }

    // ret = bpf_set_link_xdp_fd(ifindex, -1, xdp_flags);
    ret = bpf_xdp_detach(ifindex, xdp_flags, NULL);
    if (unlikely(ret < 0)) {
        error("ERR: link set xdp failed (err=%d): %s", -ret, strerror(-ret));
        return ret;
    }

    info("INFO: remove XDP prog ID:%d on ifindex:%d", curr_prog_id, ifindex);

    return 0;
}