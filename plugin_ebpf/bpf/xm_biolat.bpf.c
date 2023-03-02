/*
 * @Author: CALM.WU
 * @Date: 2023-03-02 14:05:22
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-03-02 14:19:03
 */

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"

SEC("tp_btf/block_rq_insert")
__s32 BPF_PROG(xm_tp_btf__block_rq_insert, struct request_queue *q,
               struct request *rq) {
    return 0;
}

SEC("tp_btf/block_rq_issue")
__s32 BPF_PROG(xm_tp_btf__block_rq_issue, struct request_queue *q,
               struct request *rq) {
    return 0;
}

SEC("tp_btf/block_rq_complete")
__s32 BPF_PROG(xm_tp_btf__block_rq_complete, struct request *rq, __s32 error,
               __u32 nr_bytes) {
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";