/*
 * @Author: CALM.WU
 * @Date: 2022-01-18 11:31:35
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-01-18 12:00:28
 */

#pragma once

#include <unistd.h>
#include "compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

struct process_smaps_info {
    uint64_t vmsize;
    uint64_t rss;
    uint64_t pss;
    uint64_t pss_anon;
    uint64_t pss_file;
    uint64_t pss_shmem;
    uint64_t uss;
    uint64_t swap;
};

struct process_descendant_pids {
    pid_t *pids;
    size_t pids_size;
};

extern uint32_t system_hz;
extern pid_t    system_pid_max;
extern uint32_t cpu_cores_num;

extern const char *get_hostname();

extern const char *get_ipaddr_by_iface(const char *iface, char *ip_buf, size_t ip_buf_size);

extern const char *get_macaddr_by_iface(const char *iface, char *mac_buf, size_t mac_buf_size);

extern int32_t bump_memlock_rlimit(void);

extern const char *get_username(uid_t uid);

extern uint32_t get_cpu_cores_num();

extern int32_t read_tcp_mem(uint64_t *low, uint64_t *pressure, uint64_t *high);

extern int32_t read_tcp_max_orphans(uint64_t *max_orphans);

static __always_inline int32_t get_pgsize_kb() {
    int32_t pgsize_kb = sysconf(_SC_PAGESIZE) >> 10;
    return pgsize_kb;
}

extern int32_t read_proc_pid_cmdline(pid_t pid, char *name, size_t name_size);

extern void get_system_hz();

extern uint64_t get_uptime();

extern pid_t get_system_pid_max();

extern int32_t get_process_smaps_info(pid_t pid, struct process_smaps_info *info);

// 获得进程所有的后代进程pid
extern int32_t get_process_descendant_pids(pid_t pid, struct process_descendant_pids *pd_pids);

extern int32_t get_block_device_sector_size(const char *disk);

extern int32_t get_default_gateway_and_iface(in_addr_t *addr, char *iface);

#ifdef __cplusplus
}
#endif