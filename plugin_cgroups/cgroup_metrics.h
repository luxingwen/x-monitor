/*
 * @Author: CALM.WU
 * @Date: 2022-08-22 11:00:27
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-08-22 15:28:40
 */

#pragma once

// https://docs.splunk.com/Observability/gdi/cgroups/cgroups.html

#include "prometheus-client-c/prom.h"

extern const char *sys_cgroup_v1_metric_cpu_shares_help;
extern const char *sys_cgroup_v1_metric_cpu_cfs_period_us_help;
extern const char *sys_cgroup_v1_metric_cpu_cfs_quota_us_help;   // microseconds

extern const char *sys_cgroup_v1_metric_cpuacct_stat_user_userhzs_help;
extern const char *sys_cgroup_v1_metric_cpuacct_stat_system_userhzs_help;

extern const char *sys_cgroup_v1_metric_cpu_stat_nr_periods_help;
extern const char *sys_cgroup_v1_metric_cpu_stat_nr_throttled_help;
extern const char *sys_cgroup_v1_metric_cpu_stat_throttled_time_ns_help;

extern const char *sys_cgroup_v1_metric_cpuacct_usage_ns_help;
extern const char *sys_cgroup_v1_metric_cpuacct_usage_ns_per_cpu_help;
extern const char *sys_cgroup_v1_metric_cpuacct_usage_user_ns_help;
extern const char *sys_cgroup_v1_metric_cpuacct_usage_user_ns_per_cpu_help;
extern const char *sys_cgroup_v1_metric_cpuacct_usage_system_ns_help;
extern const char *sys_cgroup_v1_metric_cpuacct_usage_system_ns_per_cpu_help;

extern const char *sys_cgroup_v1_memory_stat_cache_help;
extern const char *sys_cgroup_v1_memory_stat_rss_help;
extern const char *sys_cgroup_v1_memory_stat_rss_huge_help;
extern const char *sys_cgroup_v1_memory_stat_shmem_help;
extern const char *sys_cgroup_v1_memory_stat_mapped_file_help;
extern const char *sys_cgroup_v1_memory_stat_dirty_help;
extern const char *sys_cgroup_v1_memory_stat_writeback_help;
extern const char *sys_cgroup_v1_memory_stat_pgpgin_help;
extern const char *sys_cgroup_v1_memory_stat_pgpgout_help;
extern const char *sys_cgroup_v1_memory_stat_pgfault_help;
extern const char *sys_cgroup_v1_memory_stat_pgmajfault_help;
extern const char *sys_cgroup_v1_memory_stat_inactive_anon_help;
extern const char *sys_cgroup_v1_memory_stat_active_anon_help;
extern const char *sys_cgroup_v1_memory_stat_inactive_file_help;
extern const char *sys_cgroup_v1_memory_stat_active_file_help;
extern const char *sys_cgroup_v1_memory_stat_unevictable_help;
extern const char *sys_cgroup_v1_memory_stat_hierarchical_memory_limit_help;
extern const char *sys_cgroup_v1_memory_stat_hierarchical_memsw_limit_help;
extern const char *sys_cgroup_v1_memory_stat_swap_help;

extern const char *sys_cgroup_v1_memory_stat_total_cache_help;
extern const char *sys_cgroup_v1_memory_stat_total_rss_help;
extern const char *sys_cgroup_v1_memory_stat_total_rss_huge_help;
extern const char *sys_cgroup_v1_memory_stat_total_shmem_help;
extern const char *sys_cgroup_v1_memory_stat_total_swap_help;
extern const char *sys_cgroup_v1_memory_stat_total_mapped_file_help;
extern const char *sys_cgroup_v1_memory_stat_total_dirty_help;
extern const char *sys_cgroup_v1_memory_stat_total_writeback_help;
extern const char *sys_cgroup_v1_memory_stat_total_pgpgin_help;
extern const char *sys_cgroup_v1_memory_stat_total_pgpgout_help;
extern const char *sys_cgroup_v1_memory_stat_total_pgfault_help;
extern const char *sys_cgroup_v1_memory_stat_total_pgmajfault_help;
extern const char *sys_cgroup_v1_memory_stat_total_inactive_anon_help;
extern const char *sys_cgroup_v1_memory_stat_total_active_anon_help;
extern const char *sys_cgroup_v1_memory_stat_total_inactive_file_help;
extern const char *sys_cgroup_v1_memory_stat_total_active_file_help;
extern const char *sys_cgroup_v1_memory_stat_total_unevictable_help;

extern const char *sys_cgroup_v1_memory_usage_in_bytes_help;
extern const char *sys_cgroup_v1_memory_limit_in_bytes_help;
extern const char *sys_cgroup_v1_memory_failcnt_help;
extern const char *sys_cgroup_v1_memory_max_usage_in_bytes_help;
extern const char *sys_cgroup_v1_memory_swappiness_help;
extern const char *sys_cgroup_v1_memory_pressure_level_help;

extern const char *sys_cgroup_v2_metric_cpu_max_help;
extern const char *sys_cgroup_v2_metric_cpu_stat_nr_periods_help;
extern const char *sys_cgroup_v2_metric_cpu_stat_nr_throttled_help;
extern const char *sys_cgroup_v2_metric_cpu_stat_throttled_usec_help;
extern const char *sys_cgroup_v2_metric_cpu_stat_usage_usec_help;
extern const char *sys_cgroup_v2_metric_cpu_stat_user_usec_help;
extern const char *sys_cgroup_v2_metric_cpu_stat_system_usec_help;

extern const char *sys_cgroup_v2_metric_memory_max_help;       // memory.max，-1：表示不限制
extern const char *sys_cgroup_v2_metric_memory_current_help;   // cg当前使用的内存总量
extern const char *sys_cgroup_v2_metric_memory_stat_anon_help;
//  Amount of memory used to cache filesystem data, including tmpfs and shared memory
extern const char *sys_cgroup_v2_metric_memory_stat_file_help;
extern const char *sys_cgroup_v2_metric_memory_stat_kernel_help;
extern const char *sys_cgroup_v2_metric_memory_stat_kernel_stack_help;
extern const char *sys_cgroup_v2_metric_memory_stat_slab_help;
extern const char *sys_cgroup_v2_metric_memory_stat_sock_help;
extern const char *sys_cgroup_v2_metric_memory_stat_anon_thp_help;
extern const char *sys_cgroup_v2_metric_memory_stat_file_writeback_help;
extern const char *sys_cgroup_v2_metric_memory_stat_file_dirty_help;
extern const char *sys_cgroup_v2_metric_memory_stat_pgfault_help;
extern const char *sys_cgroup_v2_metric_memory_stat_pgmajfault_help;

extern const char *sys_cgroup_v2_metric_memory_stat_inactive_anon_help;
extern const char *sys_cgroup_v2_metric_memory_stat_active_anon_help;
extern const char *sys_cgroup_v2_metric_memory_stat_inactive_file_help;
extern const char *sys_cgroup_v2_metric_memory_stat_active_file_help;
extern const char *sys_cgroup_v2_metric_memory_stat_unevictable_help;

struct sys_cgroup_metrics {
    prom_gauge_t *sys_cgroup_v1_metric_cpu_shares;          // file cpu.shares
    prom_gauge_t *sys_cgroup_v1_metric_cpu_cfs_period_us;   // 单位：微秒 file cpu.cfs_period_us
    prom_gauge_t *sys_cgroup_v1_metric_cpu_cfs_quota_us;    // file cpu.cfs_quota_us

    prom_counter_t *sys_cgroup_v1_metric_cpu_stat_nr_periods;   // file cpuacct.cpu.stat
    prom_counter_t *sys_cgroup_v1_metric_cpu_stat_nr_throttled;
    prom_counter_t *sys_cgroup_v1_metric_cpu_stat_throttled_time_ns;

    prom_gauge_t *sys_cgroup_v1_metric_cpuacct_stat_user_userhzs;   // file cpuacct.cpuacct.stat
    prom_gauge_t *sys_cgroup_v1_metric_cpuacct_stat_system_userhzs;

    prom_counter_t *sys_cgroup_v1_metric_cpuacct_usage_ns;
    prom_counter_t *sys_cgroup_v1_metric_cpuacct_usage_user_ns;
    prom_counter_t *sys_cgroup_v1_metric_cpuacct_usage_system_ns;

    prom_counter_t *sys_cgroup_v1_metric_cpuacct_usage_ns_per_cpu;
    prom_counter_t *sys_cgroup_v1_metric_cpuacct_usage_user_ns_per_cpu;
    prom_counter_t *sys_cgroup_v1_metric_cpuacct_usage_system_ns_per_cpu;

    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_cache_bytes;   // file memory.stat
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_rss_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_rss_huge_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_shmem_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_mapped_file_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_dirty_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_writeback_bytes;

    prom_counter_t *sys_cgroup_v1_metric_memory_stat_pgpgin;
    prom_counter_t *sys_cgroup_v1_metric_memory_stat_pgpgout;
    prom_counter_t *sys_cgroup_v1_metric_memory_stat_pgfault;
    prom_counter_t *sys_cgroup_v1_metric_memory_stat_pgmajfault;

    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_inactive_anon_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_active_anon_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_inactive_file_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_active_file_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_unevictable_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_hierarchical_memory_limit_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_hierarchical_memsw_limit_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_swap_bytes;

    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_cache_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_rss_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_rss_huge_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_shmem_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_swap_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_mapped_file_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_dirty_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_writeback_bytes;

    prom_counter_t *sys_cgroup_v1_metric_memory_stat_total_pgpgin;
    prom_counter_t *sys_cgroup_v1_metric_memory_stat_total_pgpgout;
    prom_counter_t *sys_cgroup_v1_metric_memory_stat_total_pgfault;   // 缺页异常
    prom_counter_t *
        sys_cgroup_v1_metric_memory_stat_total_pgmajfault;   // 该缺页异常伴随着磁盘I/O，例如访问访问的数据还在磁盘上，就会触发一次major
                                                             // fault。

    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_inactive_anon_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_active_anon_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_inactive_file_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_active_file_bytes;
    prom_gauge_t *sys_cgroup_v1_metric_memory_stat_total_unevictable_bytes;

    // 显示当前内存（进程内存 + 页面缓存）的使用量 file memory.usage_in_bytes
    prom_gauge_t *sys_cgroup_v1_metric_memory_usage_in_bytes;
    // 显示当前内存（进程内存 + 页面缓存）使用量的限制值，超过这个值进程会被oom killer干掉或者被暂停
    // file memory.limit_in_bytes
    prom_gauge_t *sys_cgroup_v1_metric_memory_limit_in_bytes;
    // 显示内存（进程内存 + 页面缓存）达到限制值的次数
    // file memory.failcnt
    prom_gauge_t *sys_cgroup_v1_metric_memory_failcnt;
    // 历史内存最大使用量 file
    // memory.max_usage_in_bytes
    prom_gauge_t *sys_cgroup_v1_metric_memory_max_usage_in_bytes;
    // 设置、显示针对分组的swappiness（相当于sysctl的vm.swappiness）
    // file memory.swappiness
    prom_gauge_t *sys_cgroup_v1_metric_memory_swappiness;
    // 显示当前内存压力值 7: critical,
    // 3: medium, 1: low, 0: none
    prom_gauge_t *sys_cgroup_v1_metric_memory_pressure_level;

    prom_gauge_t *sys_cgroup_v2_metric_cpu_max;   // cpu资源配额，-1：表示不限制 file cpu.max

    prom_counter_t *sys_cgroup_v2_metric_cpu_stat_nr_periods;   // file cpuacct.cpu.stat
    prom_counter_t *sys_cgroup_v2_metric_cpu_stat_nr_throttled;
    prom_counter_t *sys_cgroup_v2_metric_cpu_stat_throttled_usec;   // 微秒
    prom_counter_t *sys_cgroup_v2_metric_cpu_stat_usage_usec;
    prom_counter_t *sys_cgroup_v2_metric_cpu_stat_user_usec;
    prom_counter_t *sys_cgroup_v2_metric_cpu_stat_system_usec;

    prom_gauge_t *sys_cgroup_v2_metric_memory_max_in_bytes;   // memory.max，-1：表示不限制
    prom_gauge_t *sys_cgroup_v2_metric_memory_current_in_bytes;   // cg当前使用的内存总量
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_anon_bytes;
    //  Amount of memory used to cache filesystem data, including tmpfs and shared memory
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_file_bytes;
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_kernel_bytes;
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_kernel_stack_bytes;
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_slab_bytes;
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_sock_bytes;
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_anon_thp_bytes;
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_file_writeback_bytes;
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_file_dirty_bytes;

    prom_counter_t *sys_cgroup_v2_metric_memory_stat_pgfault;
    prom_counter_t *sys_cgroup_v2_metric_memory_stat_pgmajfault;

    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_inactive_anon_bytes;
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_active_anon_bytes;
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_inactive_file_bytes;
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_active_file_bytes;
    prom_gauge_t *sys_cgroup_v2_metric_memory_stat_unevictable_bytes;
};
