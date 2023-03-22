/*
 * @Author: CALM.WU
 * @Date: 2023-03-22 11:20:35
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-22 16:36:29
 */

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_net.h"
#include "xm_bpf_helpers_maps.h"
#include "xm_bpf_helpers_math.h"

#include "../bpf_and_user.h"

// 1: os，2：namespace，3：CGroup，4：PID，5：PGID，暂时不支持cg
const volatile __s32 __filter_type = 1;
const volatile __s64 __filter_value = 0;
const volatile __u64 __block_delay_threshold_ns = 50000000; // 500ms

// 使用ringbuf上报block超时的task信息
BPF_RINGBUF(xm_offcpu_blktask_ringbuf, 1 << 24);

SEC("tp_btf/sched_switch")
__s32 BPF_PROG(xm_btp_offcpu_sched_switch, bool preempt,
               struct task_struct *prev, struct task_struct *next) {
    return 0;
}

SEC("kprobe/finish_task_switch")
__s32 BPF_KPROBE(xm_kp_offcpu_finish_task_switch, struct task_struct *prev) {
    return 0;
}

char _license[] SEC("license") = "GPL";
