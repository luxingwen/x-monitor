/*
 * @Author: CALM.WU
 * @Date: 2023-04-26 14:51:19
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-04-27 15:07:26
 */

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"

#include "../bpf_and_user.h"

#define MAP_SHARED 0x01 /* Share changes */
#define MAP_PRIVATE 0x02 /* Changes are private */
#define MAP_ANONYMOUS 0x10 /* don't use a file */

// **全局变量：过滤条件
// 过滤范围，1:os，2：namespace，3：CGroup，4：PID，5：PGID，暂时不支持cg
const volatile __s32 __filter_scope_type = 1;
// 范围具体值，例如pidnsID, pid, pgid，如果scope_type为1，表示范围为整个os
const volatile __s64 __filter_scope_value = 0;

// !! Force emitting struct event into the ELF. 切记不能使用static做修饰符
const struct xm_vmm_evt_data *__unused_vmm_evt __attribute__((unused));

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24); // 16M
} xm_vmm_event_ringbuf_map SEC(".maps");

BPF_HASH(xm_brk_shrink_map, __u32, __u64, 1024);
BPF_HASH(xm_mmap_shrink_map, __u32, __u64, 1024);

static __s32 __filter_check(struct task_struct *ts) {
    pid_t tgid = READ_KERN(ts->tgid);

    if (__filter_scope_type == 2) {
        __u32 pidns_id = __xm_get_task_pidns(ts);
        if (pidns_id != (pid_t)__filter_scope_value) {
            return -1;
        }
    } else if (__filter_scope_type == 4) {
        if (tgid != (pid_t)__filter_scope_value) {
            return -1;
        }
    } else if (__filter_scope_type == 5) {
        pid_t pgid = __xm_get_task_pgid(ts);
        if (pgid != (pid_t)__filter_scope_value) {
            return -1;
        }
    }
    return 0;
}

static void __insert_vmm_alloc_event(struct task_struct *ts,
                                     enum xm_vmm_evt_type evt_type,
                                     __u64 alloc_len) {
    struct xm_vmm_evt_data *evt = bpf_ringbuf_reserve(
        &xm_vmm_event_ringbuf_map, sizeof(struct xm_vmm_evt_data), 0);
    if (evt) {
        evt->evt_type = evt_type;
        evt->pid = __xm_get_tid();
        evt->tgid = __xm_get_pid();
        evt->len = alloc_len;
        bpf_core_read_str(evt->comm, sizeof(evt->comm), &ts->comm);
        // !!如果evt放在submit后面调用，就会被检查出内存无效
        // bpf_printk("xm_ebpf_exporter vma alloc event type:%d tgid:%d
        // len:%lu\n",
        //            evt_type, evt->tgid, evt->len);
        bpf_ringbuf_submit(evt, 0);
    }
    return;
}

SEC("kprobe/do_mmap")
__s32 xm_process_do_mmap(struct pt_regs *ctx) {
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();

    if (0 == __filter_check(ts)) {
        __u64 alloc_vm_len = (__u64)PT_REGS_PARM3(ctx);
        // 表示映射区域的保护权限，可以是PROT_NONE、PROT_READ、PROT_WRITE、PROT_EXEC，或它们的组合
        // __u64 prot = (__u64)PT_REGS_PARM4(ctx);
        // flags：表示映射区域的标志，可以是
        // MAP_SHARED、MAP_PRIVATE、MAP_FIXED、MAP_ANONYMOUS、MAP_LOCKED 等。
        __u64 flags = (__u64)PT_REGS_PARM5_CORE(ctx);

        if (flags & (MAP_PRIVATE | MAP_ANONYMOUS)) {
            // 使用mmap分配的私有匿名虚拟地址空间
            __insert_vmm_alloc_event(ts, XM_VMM_EVT_TYPE_MMAP_ANON_PRIV,
                                     alloc_vm_len);
        } else if (flags & MAP_SHARED) {
            // 使用mmap分配的共享虚拟地址空间
            __insert_vmm_alloc_event(ts, XM_VMM_EVT_TYPE_MMAP_SHARED,
                                     alloc_vm_len);
        } else {
            // 使用mmap分配的其他虚拟地址空间
            __insert_vmm_alloc_event(ts, XM_VMM_EVT_TYPE_MMAP_OTHER,
                                     alloc_vm_len);
        }
    }

    return 0;
}

SEC("kprobe/do_brk_flags")
__s32 xm_process_do_brk_flags(struct pt_regs *ctx) {
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();
    // pid_t tgid = __xm_get_pid();
    // request：表示要增加或减少的内存大小。如果该值为
    // 0，则表示不需要增加或减少内存大小，只需要查看当前数据段的结束地址。

    if (0 == __filter_check(ts)) {
        // bpf_printk("xm_ebpf_exporter process_vmm tgid:%d use brk alloc "
        //            "len:%lu heap space\n",
        //            tgid, alloc_vm_len);
        __u64 alloc_vm_len = (__u64)PT_REGS_PARM2_CORE(ctx);
        __insert_vmm_alloc_event(ts, XM_VMM_EVT_TYPE_BRK, alloc_vm_len);
    }

    return 0;
}

SEC("kprobe/" SYSCALL(sys_brk))
__s32 BPF_KPROBE(xm_process_sys_brk, __u64 brk) {
    __u64 orig_brk = 0;
    __u64 start_brk = 0;
    __u32 tid = __xm_get_tid();

    // 获取要设置堆顶地址
    __u64 new_brk = (__u64)PT_REGS_PARM1_CORE(ctx);
    // 获取task
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();
    if (0 == __filter_check(ts)) {
        // 读取task的堆顶地址
        start_brk = (__u64)BPF_CORE_READ(ts, mm, start_brk);
        orig_brk = (__u64)BPF_CORE_READ(ts, mm, brk);

        bpf_printk("xm_ebpf_exporter xm_process_sys_brk tid:%d, "
                   "start_brk:%lu, orig_brk:%lu",
                   tid, start_brk, orig_brk);
        bpf_printk("xm_ebpf_exporter xm_process_sys_brk tid:%d, "
                   "new_brk:%lu, brk:%lu",
                   tid, new_brk, brk);
        // 判断进程堆顶地址
        if (new_brk <= orig_brk) {
            // kernel会调用__do_munmap，用线程id作为标识
            // !!map的数据类型必须完全对应，否则报错
            bpf_map_update_elem(&xm_brk_shrink_map, &tid, &new_brk, BPF_ANY);
        }
    }
    return 0;
}

SEC("kprobe/__do_munmap")
__s32 xm_process_do_munmap(struct pt_regs *ctx) {
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();

    if (0 == __filter_check(ts)) {
        // 第三个参数堆释放的长度
        __u32 tid = __xm_get_tid();
        __u64 len = (__u64)PT_REGS_PARM3_CORE(ctx);
        __u64 *res = bpf_map_lookup_elem(&xm_brk_shrink_map, &tid);
        if (res) {
            bpf_printk("xm_ebpf_exporter xm_process_do_munmap tid:%d, "
                       "new_brk:0x%p, len:%lu",
                       tid, *res, len);
            bpf_map_update_elem(&xm_brk_shrink_map, &tid, &len, BPF_EXIST);
        } else {
            bpf_map_update_elem(&xm_mmap_shrink_map, &tid, &len, BPF_NOEXIST);
        }
    }

    return 0;
}

SEC("kretprobe/__do_munmap")
__s32 BPF_KRETPROBE(xm_do_mummap_exit, __s32 ret) {
    __u32 tid = __xm_get_tid();
    if (ret >= 0) {
        // 地址空间释放成功
        struct task_struct *ts = (struct task_struct *)bpf_get_current_task();

        if (0 == __filter_check(ts)) {
            __u64 *res = bpf_map_lookup_elem(&xm_brk_shrink_map, &tid);
            if (res) {
                // 释放的是堆空间
                __insert_vmm_alloc_event(ts, XM_VMM_EVT_TYPE_BRK_SHRINK, *res);
            } else {
                res = bpf_map_lookup_elem(&xm_mmap_shrink_map, &tid);
                if (res) {
                    // 释放的是mmap映射空间
                    __insert_vmm_alloc_event(ts, XM_VMM_EVT_TYPE_MUNMAP, *res);
                }
            }
        }
    }
    if (0 != bpf_map_delete_elem(&xm_mmap_shrink_map, &tid)) {
        bpf_map_delete_elem(&xm_brk_shrink_map, &tid);
    }
    return 0;
}

// ** GPL
char LICENSE[] SEC("license") = "Dual BSD/GPL";