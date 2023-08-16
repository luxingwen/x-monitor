/*
 * @Author: CALM.WU
 * @Date: 2023-08-16 15:15:40
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-08-16 16:32:31
 */

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"
#include "xm_bpf_helpers_math.h"

#include "../bpf_and_user.h"
#include "xm_bpf_helpers_filter.h"

// prog参数，过滤条件
BPF_ARRAY(xm_profile_arg_map, struct xm_prog_filter_arg, 1);
BPF_HASH(xm_profile_sample_count_map, struct xm_profile_sample, __u32,
         MAX_THREAD_COUNT);
struct {
    __uint(type, BPF_MAP_TYPE_STACK_TRACE);
    __uint(key_size, sizeof(__u32));
    __uint(value_size, PERF_MAX_STACK_DEPTH * sizeof(__u64));
    __uint(max_entries, 1024);
} xm_profile_stack_map SEC(".maps");

const enum xm_prog_filter_target_scope_type __unused_filter_scope_type
    __attribute__((unused)) = XM_PROG_FILTER_TARGET_SCOPE_TYPE_NONE;
const struct xm_profile_sample *__unused_ps __attribute__((unused));

SEC("perf_event")
__s32 xm_do_perf_event(struct bpf_perf_event_data *ctx) {
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";