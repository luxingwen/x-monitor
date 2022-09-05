/*
 * @Author: CALM.WU
 * @Date: 2022-08-22 11:00:27
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-08-22 15:28:40
 */

#pragma once

// https://docs.splunk.com/Observability/gdi/cgroups/cgroups.html

#include "prometheus-client-c/prom.h"

extern const char *__sys_cgroup_metric_cpu_shares_help;
extern const char *__sys_cgroup_metric_cpu_cfs_period_us_help;
extern const char *__sys_cgroup_metric_cpu_cfs_quota_us_help;   // microseconds
extern const char *__sys_cgroup_metric_cpu_stat_nr_periods_help;
extern const char *__sys_cgroup_metric_cpu_stat_nr_throttled_help;
extern const char *__sys_cgroup_metric_cpu_stat_throttled_time_ns_help;

extern const char *__sys_cgroup_metric_cpuacct_usage_ns_help;
extern const char *__sys_cgroup_metric_cpuacct_usage_ns_per_cpu_help;
extern const char *__sys_cgroup_metric_cpuacct_usage_user_ns_help;
extern const char *__sys_cgroup_metric_cpuacct_usage_user_ns_per_cpu_help;
extern const char *__sys_cgroup_metric_cpuacct_usage_system_ns_help;
extern const char *__sys_cgroup_metric_cpuacct_usage_system_ns_per_cpu_help;

extern const char *__sys_cgroup_memory_stat_cache_help;
extern const char *__sys_cgroup_memory_stat_rss_help;
extern const char *__sys_cgroup_memory_stat_rss_huge_help;
extern const char *__sys_cgroup_memory_stat_shmem_help;
extern const char *__sys_cgroup_memory_stat_mapped_file_help;
extern const char *__sys_cgroup_memory_stat_dirty_help;
extern const char *__sys_cgroup_memory_stat_writeback_help;
extern const char *__sys_cgroup_memory_stat_pgpgin_help;
extern const char *__sys_cgroup_memory_stat_pgpgout_help;
extern const char *__sys_cgroup_memory_stat_pgfault_help;
extern const char *__sys_cgroup_memory_stat_pgmajfault_help;
extern const char *__sys_cgroup_memory_stat_inactive_anon_help;
extern const char *__sys_cgroup_memory_stat_active_anon_help;
extern const char *__sys_cgroup_memory_stat_inactive_file_help;
extern const char *__sys_cgroup_memory_stat_active_file_help;
extern const char *__sys_cgroup_memory_stat_unevictable_help;
extern const char *__sys_cgroup_memory_stat_hierarchical_memory_limit_help;
extern const char *__sys_cgroup_memory_stat_hierarchical_memsw_limit_help;
extern const char *__sys_cgroup_memory_stat_stat_swap_help;

extern const char *__sys_cgroup_memory_stat_total_cache_help;
extern const char *__sys_cgroup_memory_stat_total_rss_help;
extern const char *__sys_cgroup_memory_stat_total_rss_huge_help;
extern const char *__sys_cgroup_memory_stat_total_shmem_help;
extern const char *__sys_cgroup_memory_stat_total_swap_help;
extern const char *__sys_cgroup_memory_stat_total_mapped_file_help;
extern const char *__sys_cgroup_memory_stat_total_dirty_help;
extern const char *__sys_cgroup_memory_stat_total_writeback_help;
extern const char *__sys_cgroup_memory_stat_total_pgpgin_help;
extern const char *__sys_cgroup_memory_stat_total_pgpgout_help;
extern const char *__sys_cgroup_memory_stat_total_pgfault_help;
extern const char *__sys_cgroup_memory_stat_total_pgmajfault_help;
extern const char *__sys_cgroup_memory_stat_total_inactive_anon_help;
extern const char *__sys_cgroup_memory_stat_total_active_anon_help;
extern const char *__sys_cgroup_memory_stat_total_inactive_file_help;
extern const char *__sys_cgroup_memory_stat_total_active_file_help;
extern const char *__sys_cgroup_memory_stat_total_unevictable_help;

extern const char *__sys_cgroup_memory_usage_in_bytes_help;
extern const char *__sys_cgroup_memory_limit_in_bytes_help;
extern const char *__sys_cgroup_memory_failcnt_help;
extern const char *__sys_cgroup_memory_max_usage_in_bytes_help;
extern const char *__sys_cgroup_memory_swappiness_help;

struct xm_cgroup_metrics {
    prom_gauge_t   *cgroup_metric_cpu_shares;
    prom_gauge_t   *cgroup_metric_cpu_cfs_period_us;   // 单位：微秒
    prom_gauge_t   *cgroup_metric_cpu_cfs_quota_us;
    prom_counter_t *cgroup_metric_cpu_stat_nr_periods;
    prom_counter_t *cgroup_metric_cpu_stat_nr_throttled;
    prom_counter_t *cgroup_metric_cpu_stat_throttled_time_ns;

    prom_counter_t *cgroup_metric_cpuacct_usage_ns;
    prom_counter_t *cgroup_metric_cpuacct_usage_ns_per_cpu;
    prom_counter_t *cgroup_metric_cpuacct_usage_user_ns;
    prom_counter_t *cgroup_metric_cpuacct_usage_user_ns_per_cpu;
    prom_counter_t *cgroup_metric_cpuacct_usage_system_ns;
    prom_counter_t *cgroup_metric_cpuacct_usage_system_ns_per_cpu;

    prom_gauge_t *cgroup_metric_memory_stat_cache_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_rss_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_rss_huge_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_shmem_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_mapped_file_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_dirty_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_writeback_bytes;

    prom_counter_t *cgroup_metric_memory_stat_pgpgin;
    prom_counter_t *cgroup_metric_memory_stat_pgpgout;
    prom_counter_t *cgroup_metric_memory_stat_pgfault;
    prom_counter_t *cgroup_metric_memory_stat_pgmajfault;

    prom_gauge_t *cgroup_metric_memory_stat_inactive_anon_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_active_anon_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_inactive_file_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_active_file_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_unevictable_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_hierarchical_memory_limit_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_hierarchical_memsw_limit_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_swap_bytes;

    prom_gauge_t *cgroup_metric_memory_stat_total_cache_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_total_rss_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_total_rss_huge_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_total_shmem_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_total_swap_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_total_mapped_file_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_total_dirty_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_total_writeback_bytes;

    prom_counter_t *cgroup_metric_memory_stat_total_pgpgin;
    prom_counter_t *cgroup_metric_memory_stat_total_pgpgout;
    prom_counter_t *cgroup_metric_memory_stat_total_pgfault;   // 缺页异常
    prom_counter_t *
        cgroup_metric_memory_stat_total_pgmajfault;   // 该缺页异常伴随着磁盘I/O，例如访问访问的数据还在磁盘上，就会触发一次major
                                                      // fault。

    prom_gauge_t *cgroup_metric_memory_stat_total_inactive_anon_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_total_active_anon_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_total_inactive_file_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_total_active_file_bytes;
    prom_gauge_t *cgroup_metric_memory_stat_total_unevictable_bytes;

    prom_gauge_t
        *cgroup_metric_memory_usage_in_bytes;   // 显示当前内存（进程内存 + 页面缓存）的使用量
    prom_gauge_t
        *cgroup_metric_memory_limit_in_bytes;   // 显示当前内存（进程内存 +
                                                // 页面缓存）使用量的限制值，超过这个值进程会被oom
                                                // killer干掉或者被暂停
    prom_gauge_t *cgroup_metric_memory_failcnt;   // 显示内存（进程内存 + 页面缓存）达到限制值的次数
    prom_gauge_t *cgroup_metric_memory_max_usage_in_bytes;   // 历史内存最大使用量
    prom_gauge_t *
        cgroup_metric_memory_swappiness;   // 设置、显示针对分组的swappiness（相当于sysctl的vm.swappiness）
};
