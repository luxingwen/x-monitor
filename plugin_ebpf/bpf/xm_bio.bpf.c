/*
 * @Author: CALM.WU
 * @Date: 2023-07-03 10:07:51
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-07-03 10:19:19
 */

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"

#include "../bpf_and_user.h"

#define MAX_REQ_COUNT 10240

extern int LINUX_KERNEL_VERSION __kconfig;

// 是否按每个request的cmd_flags进行过滤
const volatile bool __filter_per_cmd_flag = 1;

struct bio_req_stage {
    __u64 insert;
    __u64 issue;
};

BPF_HASH(xm_bio_request_start_map, struct request *, struct bio_req_stage,
         10240);
BPF_HASH(xm_bio_request_latency_hists_map, struct xm_bio_req_latency_hist_key,
         struct xm_bio_req_latency_hist, 1024); // op枚举乘以8个分区

// ** 类型标识，为了bpf2go程序生成golang类型
static struct xm_bio_req_latency_hist_key __zero_bio_req_latency_hist_key = {
    .major = 0,
    .first_minor = 0,
    .cmd_flags = 0,
};

static struct xm_bio_req_latency_hist __zero_bio_req_latency_hist = {
    .slots = { 0 },
};

static __always_inline __s32 xm_trace_request(struct request *rq, bool insert) {
    struct bio_req_stage *stage_p = NULL, init_stage = { 0, 0 };
    __u64 now_ns = bpf_ktime_get_ns();

    // 查找
    stage_p = bpf_map_lookup_elem(&xm_bio_request_start_map, &rq);
    if (!stage_p) {
        // request不存在
        stage_p = &init_stage;
    }
    if (insert) {
        // 记录request 插入dispatch队列的时间
        stage_p->insert = now_ns;
    } else {
        // 记录request 从dispatch队列出来放入驱动队列的事件，开始执行
        stage_p->issue = now_ns;
    }

    if (stage_p == &init_stage) {
        // 相等只有在request不存在的时候
        bpf_map_update_elem(&xm_bio_request_start_map, &rq, stage_p,
                            BPF_NOEXIST);
    }
    return 0;
}

// 在插入io-scheduler的dispatch队列之前触发这个tracepoint
// 因为在kernel 5.11.0之后，这个tracepoint的参数只有struct request了，
// 所以这里没法用BPF_PROG宏，需直接使用ctx作为参数，根据内核版本来获取参数
SEC("tp_btf/block_rq_insert") __s32 xm_tp_btf__block_rq_insert(__u64 *ctx) {
    struct request *rq = NULL;

    if (LINUX_KERNEL_VERSION < KERNEL_VERSION(5, 11, 0)) {
        rq = (struct request *)ctx[1];
    } else {
        rq = (struct request *)ctx[0];
    }
    xm_trace_request(rq, true);
    return 0;
}

SEC("tp_btf/block_rq_issue")
__s32 xm_tp_btf__block_rq_issue(__u64 *ctx) {
    struct request *rq = NULL;

    if (LINUX_KERNEL_VERSION < KERNEL_VERSION(5, 11, 0)) {
        rq = (struct request *)ctx[1];
    } else {
        rq = (struct request *)ctx[0];
    }
    xm_trace_request(rq, false);
    return 0;
}

SEC("tp_btf/block_rq_complete")
__s32 BPF_PROG(xm_tp_btf__block_rq_complete, struct request *rq, __s32 error,
               __u32 nr_bytes) {
    return 0;
}

SEC("kprobe/blk_account_io_start")
__s32 BPF_KPROBE(kprobe__xm_blk_account_io_start, struct request *rq) {
    return 0;
}

SEC("kprobe/blk_account_io_merge_bio")
__s32 BPF_KPROBE(kprobe__xm_blk_account_io_merge_bio, struct request *rq) {
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
