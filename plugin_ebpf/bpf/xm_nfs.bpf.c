/*
 * @Author: CALM.WU
 * @Date: 2024-02-29 17:04:07
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-02-29 17:28:52
 */

#include "vmlinux.h"
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"

//-----read-----

SEC("kprobe/nfs_file_read")
__s32 BPF_KPROBE(xm_kp_nfs_file_read, struct kiocb *iocb, struct iov_iter *to) {
    return 0;
}

SEC("kretprobe/nfs_file_read")
__s32 BPF_KRETPROBE(xm_krp_nfs_file_read, __s64 ret) {
    return 0;
}

//-----open-----

SEC("kprobe/nfs_file_open")
__s32 BPF_KPROBE(xm_kp_nfs_file_open, struct inode *inode, struct file *filp) {
    return 0;
}

SEC("kretprobe/nfs_file_open")
__s32 BPF_KRETPROBE(xm_krp_nfs_file_open, __s64 ret) {
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