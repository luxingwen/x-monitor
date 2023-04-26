/*
 * @Author: CALM.WU
 * @Date: 2023-04-26 14:51:19
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-04-26 14:52:13
 */

#include <vmlinux.h>
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"

#include "../bpf_and_user.h"

#define MAP_PRIVATE 0x02 /* Changes are private */
#define MAP_ANONYMOUS 0x10 /* don't use a file */

// **全局变量：过滤条件
// 过滤范围，1:os，2：namespace，3：CGroup，4：PID，5：PGID，暂时不支持cg
const volatile __s32 __filter_scope_type = 1;
// 范围具体值，例如pidnsID, pid, pgid，如果scope_type为1，表示范围为整个os
const volatile __s64 __filter_scope_value = 0;

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24); // 16M
} xm_pvm_event_ringbuf_map SEC(".maps");

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

static void __insert_pvm_alloc_event() {
    return;
}

SEC("kprobe/do_mmap")
__s32 xm_process_vm_do_mmap(struct pt_regs *ctx) {
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();
    pid_t tgid = __xm_get_pid();

    //__u64 alloc_vm_addr = (__u64)PT_REGS_PARM2(ctx);
    __u64 alloc_vm_len = (__u64)PT_REGS_PARM3(ctx);
    // 表示映射区域的保护权限，可以是PROT_NONE、PROT_READ、PROT_WRITE、PROT_EXEC，或它们的组合
    // __u64 prot = (__u64)PT_REGS_PARM4(ctx);
    // flags：表示映射区域的标志，可以是
    // MAP_SHARED、MAP_PRIVATE、MAP_FIXED、MAP_ANONYMOUS、MAP_LOCKED 等。
    __u64 flags = (__u64)PT_REGS_PARM5(ctx);

    if (flags & (MAP_PRIVATE | MAP_ANONYMOUS)) {
        // 使用mmap分配的私有匿名虚拟地址空间
        if (0 == __filter_check(ts)) {
            bpf_printk("xm_ebpf_exporter process_vm tgid:%d use mmap alloc "
                       "len:%lu private anonymous space\n",
                       tgid, alloc_vm_len);
            __insert_pvm_alloc_event();
        }
    }

    return 0;
}

SEC("kprobe/do_brk_flags")
__s32 xm_process_vm_do_brk_flags(struct pt_regs *ctx) {
    struct task_struct *ts = (struct task_struct *)bpf_get_current_task();
    pid_t tgid = __xm_get_pid();
    // request：表示要增加或减少的内存大小。如果该值为
    // 0，则表示不需要增加或减少内存大小，只需要查看当前数据段的结束地址。
    __u64 alloc_vm_len = (__u64)PT_REGS_PARM2(ctx);

    if (0 == __filter_check(ts)) {
        bpf_printk("xm_ebpf_exporter process_vm tgid:%d use brk alloc "
                   "len:%lu heap space\n",
                   tgid, alloc_vm_len);
        __insert_pvm_alloc_event();
    }

    return 0;
}