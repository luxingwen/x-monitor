/*
 * @Author: CALM.WU
 * @Date: 2023-02-22 14:01:14
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-22 14:43:17
 */

// https://github.com/wubo0067/x-monitor/blob/feature-xm-ebpf-collector/doc/cachestat.md

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "../bpf_and_user.h"

volatile __s64 xm_cs_total = 0; /* mark_page_accessed() - number of
                                    mark_buffer_dirty() */
volatile __s64 xm_cs_misses = 0; /* add_to_page_cache_lru() - number of
                                  * account_page_dirtied() */
volatile __s64 xm_cs_mbd = 0; /* total of mark_buffer_dirty events */

SEC("kprobe/add_to_page_cache_lru")
__s32 BPF_KPROBE(xm_kp_cs_atpcl) {
    __sync_fetch_and_add(&xm_cs_misses, 1);
    return 0;
}

SEC("kprobe/mark_page_accessed")
__s32 BPF_KPROBE(xm_kp_cs_mpa) {
    __sync_fetch_and_add(&xm_cs_total, 1);
    return 0;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 15, 0))
SEC("kprobe/folio_account_dirtied")
__s32 BPF_KPROBE(xm_kp_cs_fad) {
    __sync_fetch_and_add(&xm_cs_misses, -1);
    return 0;
}
#else
SEC("kprobe/account_page_dirtied")
__s32 BPF_KPROBE(xm_kp_cs_apd) {
    __sync_fetch_and_add(&xm_cs_misses, -1);
    return 0;
}
#endif

SEC("kprobe/mark_buffer_dirty")
__s32 BPF_KPROBE(xm_kp_cs_mbd) {
    __sync_fetch_and_add(&xm_cs_total, -1);
    __sync_fetch_and_add(&xm_cs_mbd, 1);
    return 0;
}

char _license[] SEC("license") = "GPL";