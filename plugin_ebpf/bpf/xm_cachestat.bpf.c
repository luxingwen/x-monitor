/*
 * @Author: CALM.WU
 * @Date: 2023-02-22 14:01:14
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-22 18:24:09
 */

// https://github.com/wubo0067/x-monitor/blob/feature-xm-ebpf-collector/doc/cachestat.md

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"
#include "../bpf_and_user.h"

volatile __s64 xm_cs_total = 0; /* mark_page_accessed() - number of
                                    mark_buffer_dirty() */
volatile __s64 xm_cs_misses = 0; /* add_to_page_cache_lru() - number of
                                  * account_page_dirtied() */
volatile __s64 xm_cs_mbd = 0; /* total of mark_buffer_dirty events */

BPF_HASH(xm_page_cache_ops_count, __u64, __u64, 4);

SEC("kprobe/add_to_page_cache_lru")
__s32 BPF_KPROBE(xm_kp_cs_atpcl) {
    __u64 ip = KPROBE_REGS_IP_FIX(PT_REGS_IP_CORE(ctx));
    __xm_bpf_map_increment(&xm_page_cache_ops_count, &ip, 1);
    //__sync_fetch_and_add(&xm_cs_misses, 1);
    return 0;
}

SEC("kprobe/mark_page_accessed")
__s32 BPF_KPROBE(xm_kp_cs_mpa) {
    __u64 ip = PT_REGS_IP(ctx);
    __xm_bpf_map_increment(&xm_page_cache_ops_count, &ip, 1);
    //__sync_fetch_and_add(&xm_cs_total, 1);
    return 0;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 15, 0))
SEC("kprobe/folio_account_dirtied")
__s32 BPF_KPROBE(xm_kp_cs_fad) {
    __u64 ip = PT_REGS_IP(ctx);
    __xm_bpf_map_increment(&xm_page_cache_ops_count, &ip, 1);
    return 0;
}
#else
SEC("kprobe/account_page_dirtied")
__s32 BPF_KPROBE(xm_kp_cs_apd) {
    __u64 ip = PT_REGS_IP(ctx);
    __xm_bpf_map_increment(&xm_page_cache_ops_count, &ip, 1);
    //__sync_fetch_and_add(&xm_cs_misses, -1);
    return 0;
}
#endif

SEC("kprobe/mark_buffer_dirty")
__s32 BPF_KPROBE(xm_kp_cs_mbd) {
    __u64 ip = PT_REGS_IP(ctx);
    __xm_bpf_map_increment(&xm_page_cache_ops_count, &ip, 1);
    // __sync_fetch_and_add(&xm_cs_total, -1);
    // __sync_fetch_and_add(&xm_cs_mbd, 1);
    return 0;
}

char _license[] SEC("license") = "GPL";