/*
 * @Author: CALM.WU
 * @Date: 2023-07-03 10:07:51
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-07-10 15:28:46
 */

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"
#include "xm_bpf_helpers_math.h"

#include "../bpf_and_user.h"

#define MAX_REQ_COUNT 10240
#define REQ_OP_BITS 8
#define REQ_OP_MASK ((1 << REQ_OP_BITS) - 1)

extern int LINUX_KERNEL_VERSION __kconfig;

// 是否按每个request的cmd_flags进行过滤
const volatile bool __filter_per_cmd_flag = false;
const volatile __s64 __request_latency_min_ns = 1000000; // 1ms

struct bio_req_stage {
    __u64 insert;
    __u64 issue;
};

struct bio_pid_data {
    pid_t tid; // 线程id
    pid_t pid; // 进程id
    char comm[TASK_COMM_LEN];
};

BPF_HASH(xm_bio_request_pid_data_map, struct request *, struct bio_pid_data,
         10240);
BPF_HASH(xm_bio_request_start_map, struct request *, struct bio_req_stage,
         10240);
BPF_HASH(xm_bio_info_map, struct xm_bio_key, struct xm_bio_data,
         1024); // op枚举乘以8个分区
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 16); // 64k
} xm_bio_req_latency_event_ringbuf_map SEC(".maps");

// ** 类型标识，为了bpf2go程序生成golang类型
static struct xm_bio_key __zero_bio_key __attribute__((unused)) = {
    .major = 0,
    .first_minor = 0,
    .cmd_flags = 0,
};

const struct xm_bio_request_latency_evt_data *__unused_brl_evt
    __attribute__((unused));

static struct xm_bio_data __zero_bio_info = {
    .req_latency_in2c_slots = { 0 },
    .req_latency_is2c_slots = { 0 },
    .bytes = 0,
    .sequential_count = 0,
    .random_count = 0,
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
        __s64 delta = (__s64)(now_ns - stage_p->insert);
        if (delta < 0) {
            bpf_map_delete_elem(&xm_bio_request_start_map, &rq);
            return 0;
        }
    }

    if (stage_p == &init_stage) {
        // 相等只有在request不存在的时候
        bpf_map_update_elem(&xm_bio_request_start_map, &rq, stage_p,
                            BPF_NOEXIST);
    }
    return 0;
}

SEC("kprobe/blk_account_io_merge_bio")
__s32 BPF_KPROBE(kprobe__blk_account_io_merge_bio, struct request *rq) {
    struct bio_pid_data bpd;
    bpd.pid = __xm_get_pid();
    bpd.tid = __xm_get_tid();
    bpf_get_current_comm(&bpd.comm, sizeof(bpd.comm));
    bpf_map_update_elem(&xm_bio_request_pid_data_map, &rq, &bpd, 0);
    return 0;
}

SEC("kprobe/blk_account_io_start")
__s32 BPF_KPROBE(kprobe__blk_account_io_start, struct request *rq) {
    struct bio_pid_data bpd;
    bpd.pid = __xm_get_pid();
    bpd.tid = __xm_get_tid();
    bpf_get_current_comm(&bpd.comm, sizeof(bpd.comm));
    bpf_map_update_elem(&xm_bio_request_pid_data_map, &rq, &bpd, 0);
    return 0;
}

// 在插入io-scheduler的dispatch队列之前触发这个tracepoint
// 因为在kernel 5.11.0之后，这个tracepoint的参数只有struct request了，
// 所以这里没法用BPF_PROG宏，需直接使用ctx作为参数，根据内核版本来获取参数
SEC("tp_btf/block_rq_insert") __s32 xm_tp_btf__block_rq_insert(__u64 *ctx) {
    if (LINUX_KERNEL_VERSION < KERNEL_VERSION(5, 11, 0)) {
        xm_trace_request((struct request *)ctx[1], true);
        // !! 这里这样写导致了bpf_load失败
        // rq = (struct request *)ctx[1];
    } else {
        xm_trace_request((struct request *)ctx[0], true);
    }
    return 0;
}

SEC("tp_btf/block_rq_issue")
__s32 xm_tp_btf__block_rq_issue(__u64 *ctx) {
    if (LINUX_KERNEL_VERSION < KERNEL_VERSION(5, 11, 0)) {
        xm_trace_request((struct request *)ctx[1], false);
        // rq = (struct request *)ctx[1];
    } else {
        xm_trace_request((struct request *)ctx[0], false);
    }
    return 0;
}

/**
 * block_rq_complete - block IO operation completed by device driver
 * @rq: block operations request
 * @error: status code
 * @nr_bytes: number of completed bytes
 */
SEC("tp_btf/block_rq_complete")
__s32 BPF_PROG(xm_tp_btf__block_rq_complete, struct request *rq, __s32 error,
               __u32 nr_bytes) {
    __s64 delta, delta_in2c, delta_is2c;
    struct xm_bio_key bio_key = { 0, 0, 0 };
    __u64 now_ns = bpf_ktime_get_ns();
    // 请求的扇区号
    sector_t sector;
    // 操作的扇区数量
    __u32 nr_sector;
    //
    __u64 slot;

    struct bio_req_stage *stage_p =
        bpf_map_lookup_elem(&xm_bio_request_start_map, &rq);
    if (!stage_p) {
        // 如果在complete回调中找不到该request，直接返回
        return 0;
    }

    // 计算延迟时间
    delta = (__s64)(now_ns - stage_p->insert);
    if (delta < 0) {
        goto cleanup;
    }
    // 转换为微秒
    delta_in2c = delta / 1000U;

    delta = (__s64)(now_ns - stage_p->issue);
    if (delta < 0) {
        goto cleanup;
    }
    delta_is2c = delta / 1000U;

    // 获得磁盘信息
    struct gendisk *disk = __xm_get_disk(rq);
    if (disk) {
        // 构建key
        bio_key.major = BPF_CORE_READ(disk, major);
        bio_key.first_minor = BPF_CORE_READ(disk, first_minor);
        if (__filter_per_cmd_flag) {
            // 读取cmd_flags
            // bio_key.cmd_flags = BPF_CORE_READ(rq, cmd_flags) | REQ_OP_MASK;
            bio_key.cmd_flags = rq->cmd_flags & REQ_OP_MASK;
        }
    } else {
        goto cleanup;
    }

    // 查询
    struct xm_bio_data *bio_info_p = __xm_bpf_map_lookup_or_try_init(
        &xm_bio_info_map, &bio_key, &__zero_bio_info);
    if (bio_info_p) {
        // 请求的起始扇区号
        sector = READ_KERN(rq->__sector);
        // 操作的连续扇区数量
        nr_sector = nr_bytes >> 9;
        if (bio_info_p->last_sector) {
            // 判断上次请求的最后扇区号
            if (bio_info_p->last_sector == sector) {
                // 循序操作
                __xm_update_u64(&bio_info_p->sequential_count, 1);
            } else {
                // 随机操作
                __xm_update_u64(&bio_info_p->random_count, 1);
            }
            // 累计块设备的操作字节数
            __xm_update_u64(&bio_info_p->bytes, nr_bytes);
        }
        // 计算last sector
        bio_info_p->last_sector = sector + nr_sector;

        // 统计blk request错误次数
        if (error < 0) {
            __xm_update_u64(&bio_info_p->req_err_count, 1);
        }

        // 计算延时槽位
        slot = __xm_log2l(delta_in2c);
        if (slot >= XM_BIO_REQ_LATENCY_MAX_SLOTS) {
            slot = XM_BIO_REQ_LATENCY_MAX_SLOTS - 1;
        }
        __sync_fetch_and_add(&bio_info_p->req_latency_in2c_slots[slot], 1);

        slot = __xm_log2l(delta_is2c);
        if (slot >= XM_BIO_REQ_LATENCY_MAX_SLOTS) {
            slot = XM_BIO_REQ_LATENCY_MAX_SLOTS - 1;
        }
        __sync_fetch_and_add(&bio_info_p->req_latency_is2c_slots[slot], 1);

        // 判断是否超过阈值
        if (delta >= __request_latency_min_ns) {
            struct xm_bio_request_latency_evt_data *evt = bpf_ringbuf_reserve(
                &xm_bio_req_latency_event_ringbuf_map,
                sizeof(struct xm_bio_request_latency_evt_data), 0);
            if (evt) {
                // 进程信息需要从xm_bio_request_pid_data_map来获取
                struct bio_pid_data *bpd =
                    bpf_map_lookup_elem(&xm_bio_request_pid_data_map, &rq);
                if (!bpd) {
                    evt->comm[0] = '?';
                } else {
                    evt->tid = bpd->tid;
                    evt->pid = bpd->pid;
                    __builtin_memcpy(evt->comm, bpd->comm, sizeof(evt->comm));
                }
                evt->major = bio_key.major;
                evt->first_minor = bio_key.first_minor;
                evt->cmd_flags = bio_key.cmd_flags;
                evt->len = nr_bytes;
                evt->in2c_micro_secs = delta_in2c;
                evt->req_in_queue_micro_secs = delta_in2c - delta_is2c;
                bpf_ringbuf_submit(evt, 0);
            }
        }
    }

cleanup:
    bpf_map_delete_elem(&xm_bio_request_start_map, &rq);
    bpf_map_delete_elem(&xm_bio_request_pid_data_map, &rq);

    return 0;
}

// completed error code
#if 0
static const struct {
    int errno;
    const char *name;
} blk_errors[] = {
    [BLK_STS_OK] = { 0, "" },
    [BLK_STS_NOTSUPP] = { -EOPNOTSUPP, "operation not supported" },
    [BLK_STS_TIMEOUT] = { -ETIMEDOUT, "timeout" },
    [BLK_STS_NOSPC] = { -ENOSPC, "critical space allocation" },
    [BLK_STS_TRANSPORT] = { -ENOLINK, "recoverable transport" },
    [BLK_STS_TARGET] = { -EREMOTEIO, "critical target" },
    [BLK_STS_NEXUS] = { -EBADE, "critical nexus" },
    [BLK_STS_MEDIUM] = { -ENODATA, "critical medium" },
    [BLK_STS_PROTECTION] = { -EILSEQ, "protection" },
    [BLK_STS_RESOURCE] = { -ENOMEM, "kernel resource" },
    [BLK_STS_DEV_RESOURCE] = { -EBUSY, "device resource" },
    [BLK_STS_AGAIN] = { -EAGAIN, "nonblocking retry" },

    /* device mapper special case, should not leak out: */
    [BLK_STS_DM_REQUEUE] = { -EREMCHG, "dm internal retry" },

    /* zone device specific errors */
    [BLK_STS_ZONE_OPEN_RESOURCE] = { -ETOOMANYREFS, "open zones exceeded" },
    [BLK_STS_ZONE_ACTIVE_RESOURCE] = { -EOVERFLOW, "active zones exceeded" },

    /* everything else not covered above: */
    [BLK_STS_IOERR] = { -EIO, "I/O" },
}
#endif

char LICENSE[] SEC("license") = "Dual BSD/GPL";
