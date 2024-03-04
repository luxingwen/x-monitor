/*
 * @Author: CALM.WU
 * @Date: 2024-02-29 17:04:07
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-03-04 16:09:17
 */

#include "vmlinux.h"
#include "../bpf_and_user.h"
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"
#include "xm_bpf_helpers_hard.h"

// 定义 ebpf map
// 用来记录 pid 的 nfs 操作的开始和结束
BPF_PERCPU_HASH(xm_nfs_op_tracing_map, __u32, __u64, 10240);
// 支持 256 个 nfs 盘
BPF_PERCPU_HASH(xm_nfs_oplat_hist_map, struct xm_blk_num,
                struct xm_nfs_oplat_stat, 256);

// 初始化变量
static struct xm_blk_num __init_blk_num __attribute__((unused)) = {
    .major = 0,
    .minor = 0,
};

static struct xm_nfs_oplat_stat __init_nfs_oplat_stat
    __attribute__((unused)) = {
        .slots = { 0 },
    };

//-----open-----

SEC("kprobe/nfs_file_open")
__s32 BPF_KPROBE(xm_kp_nfs_file_open, struct inode *inode, struct file *filp) {
    return 0;
}

SEC("kretprobe/nfs_file_open")
__s32 BPF_KRETPROBE(xm_krp_nfs_file_open, __s64 ret) {
    return 0;
}

//-----read-----

SEC("kprobe/nfs_file_read")
__s32 BPF_KPROBE(xm_kp_nfs_file_read, struct kiocb *iocb, struct iov_iter *to) {
    return 0;
}

SEC("kretprobe/nfs_file_read")
__s32 BPF_KRETPROBE(xm_krp_nfs_file_read, __s64 ret) {
    return 0;
}

//-----write-----

SEC("kprobe/nfs_file_write")
__s32 BPF_KPROBE(xm_kp_nfs_file_write, struct kiocb *iocb,
                 struct iov_iter *from) {
    return 0;
}

SEC("kretprobe/nfs_file_write")
__s32 BPF_KRETPROBE(xm_krp_nfs_file_write, __s64 ret) {
    return 0;
}

//-----getattr-----

SEC("kprobe/nfs_getattr")
__s32 BPF_KPROBE(xm_kp_nfs_getattr, struct path *path, struct kstat *stat,
                 __u32 request_mask, __u32 flags) {
    return 0;
}

SEC("kretprobe/nfs_getattr")
__s32 BPF_KRETPROBE(xm_krp_nfs_getattr, __s64 ret) {
    return 0;
}

char _license[] SEC("license") = "GPL";