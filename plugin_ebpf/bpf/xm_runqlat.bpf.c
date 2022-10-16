/*
 * @Author: calmwu
 * @Date: 2022-02-28 14:24:40
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-10-14 14:39:45
 */

#include <vmlinux.h>
#include "xm_bpf_common.h"
#include "xm_bpf_parsing_helpers.h"

#include "../bpf_and_user.h"

// 检查系统中CPU调度器延迟（运行队列的延迟，rbtree）。在需求超过供给，CPU资源处于饱和状态时，runqlat统计的信息是每个线程等待CPU的耗时
// 这里使用btf raw tracepoint，其和常规的raw tracepoint有一定的差别
// https://mozillazg.com/2022/06/ebpf-libbpf-btf-powered-enabled-raw-tracepoint-common-questions.html#hidsec
//

SEC("tp_btf/sched_wakeup")
__s32 BPF_PROG(xm_btp_sched_wakeup, struct task_struct *ts) {
    return 0;
}

SEC("tp_btf/sched_wakeup_new")
__s32 BPF_PROG(xm_btp_sched_wakeup_new, struct task_struct *ts) {
    return 0;
}

SEC("tp_btf/sched_switch")
__s32 BPF_PROG(xm_btp_sched_switch, struct task_struct *ts) {
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
