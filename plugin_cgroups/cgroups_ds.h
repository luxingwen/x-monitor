/*
 * @Author: CALM.WU
 * @Date: 2022-08-22 14:58:18
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-08-22 15:08:24
 */

#pragma once

#include <stdint.h>
#include <sys/types.h>

struct sys_cgroup_counters {
    uint64_t cpu_stat_nr_periods;
    uint64_t cpu_stat_nr_throttled;
    uint64_t cpu_stat_throttled_time_ns;

    uint64_t cpuacct_usage_ns;
    uint64_t cpuacct_usage_ns_per_cpu;
    uint64_t cpuacct_usage_user_ns;
    uint64_t cpuacct_usage_user_ns_per_cpu;
    uint64_t cpuacct_usage_system_ns;
    uint64_t cpuacct_usage_system_ns_per_cpu;

    uint64_t memory_stat_pgpgin;
    uint64_t memory_stat_pgpgout;
    uint64_t memory_stat_pgfault;
    uint64_t memory_stat_pgmajfault;

    uint64_t memory_stat_total_pgpgin;
    uint64_t memory_stat_total_pgpgout;
    uint64_t memory_stat_total_pgfault;
    uint64_t memory_stat_total_pgmajfault;
};