/*
 * @Author: calmwu
 * @Date: 2022-02-28 14:38:26
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-10-14 14:39:39
 */

// 采样CPU运行队列的长度信息，可以统计有多少个线程正在等待运行

#include <vmlinux.h>
#include "xm_bpf_common.h"
#include "xm_bpf_parsing_helpers.h"

#include "../bpf_and_user.h"

SEC("perf_event")
__s32 xm_bperf_runqlen_do_sample(struct bpf_perf_event_data *ctx) {
    return 0;
}

char LICENSE[] SEC("license") = "GPL";