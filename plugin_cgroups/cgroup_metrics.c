/*
 * @Author: CALM.WU
 * @Date: 2022-08-22 11:28:44
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-08-22 15:29:49
 */

// http://man7.org/linux/man-pages/man7/cgroups.7.html
// https://segmentfault.com/a/1190000008323952?utm_source=sf-similar-article

#include "cgroup_metrics.h"

const char *__sys_cgroup_metric_cpu_shares_help =
    "The relative share of CPU that this cgroup gets. This number is divided into the sum total of "
    "all cpu share values to determine the share any individual cgroup is entitled to.";
const char *__sys_cgroup_metric_cpu_cfs_period_us_help =
    "The period of time in microseconds for how regularly a cgroup's access to CPU resources "
    "should be reallocated. This value is used by the Completely Fair Scheduler to determine how "
    "to share CPU time between cgroups.";
const char *__sys_cgroup_metric_cpu_cfs_quota_us_help =
    "The total amount of time in microseconds for which all tasks in a cgroup can run during one "
    "period. The period is in the metric `cgroup.cpu_cfs_period_us`";

const char *__sys_cgroup_metric_cpuacct_stat_user_userhzs_help =
    "The number of USER_HZ units of time that this cgroup's tasks have been scheduled in user "
    "mode.";
const char *__sys_cgroup_metric_cpuacct_stat_system_userhzs_help =
    "The number of USER_HZ units of time that this cgroup's tasks have been scheduled in kernel "
    "mode.";

const char *__sys_cgroup_metric_cpu_stat_nr_periods_help =
    "Number of period intervals that have elapsed (the period length is in the metric "
    "`cgroup.cpu_cfs_period_us`)";
const char *__sys_cgroup_metric_cpu_stat_nr_throttled_help =
    "Number of times tasks in this cgroup have been throttled (that is, not allowed to run as much "
    "as they want).";
const char *__sys_cgroup_metric_cpu_stat_throttled_time_ns_help =
    "The total time in nanoseconds for which tasks in a cgroup have been throttled.";
const char *__sys_cgroup_metric_cpuacct_usage_ns_help =
    "Total time in nanoseconds spent using any CPU by tasks in this cgroup";
const char *__sys_cgroup_metric_cpuacct_usage_ns_per_cpu_help =
    "Total time in nanoseconds spent using a specific CPU (core) by tasks in this cgroup. This "
    "metric will have the `cpu` dimension that specifies the specific cpu/core.";
const char *__sys_cgroup_metric_cpuacct_usage_user_ns_help =
    "Total time in nanoseconds spent in user mode on any CPU by tasks in this cgroup";
const char *__sys_cgroup_metric_cpuacct_usage_user_ns_per_cpu_help =
    "Total time in nanoseconds spent in user mode on a specific CPU (core) by tasks in this "
    "cgroup. This metric will have the `cpu` dimension that specifies the specific cpu/core.";
const char *__sys_cgroup_metric_cpuacct_usage_system_ns_help =
    "Total time in nanoseconds spent in system (kernel) mode on any CPU by tasks in this cgroup";
const char *__sys_cgroup_metric_cpuacct_usage_system_ns_per_cpu_help =
    "Total time in nanoseconds spent in system (kernel) mode on a specific CPU (core) by tasks in "
    "this cgroup. This metric will have the `cpu` dimension that specifies the specific cpu/core.";

const char *__sys_cgroup_memory_stat_cache_help = "Page cache, including tmpfs (shmem), in bytes";
const char *__sys_cgroup_memory_stat_rss_help =
    "Anonymous and swap cache, not including tmpfs (shmem), in bytes";
const char *__sys_cgroup_memory_stat_rss_huge_help = "Bytes of anonymous transparent hugepages";
const char *__sys_cgroup_memory_stat_shmem_help = "Bytes of shared memory";
const char *__sys_cgroup_memory_stat_mapped_file_help =
    "Bytes of mapped file (includes tmpfs/shmem)";
const char *__sys_cgroup_memory_stat_dirty_help =
    "Bytes that are waiting to get written back to the disk";
const char *__sys_cgroup_memory_stat_writeback_help =
    "Bytes of file/anon cache that are queued for syncing to disk";
const char *__sys_cgroup_memory_stat_pgpgin_help =
    "Number of charging events to the memory cgroup. The charging event happens each time a page "
    "is accounted as either mapped anon page(RSS) or cache page(Page Cache) to the cgroup.";
const char *__sys_cgroup_memory_stat_pgpgout_help =
    "Number of uncharging events to the memory cgroup. The uncharging event happens each time a "
    "page is unaccounted from the cgroup.";
const char *__sys_cgroup_memory_stat_pgfault_help = "Total number of page faults incurred";
const char *__sys_cgroup_memory_stat_pgmajfault_help = "Number of major page faults incurred";

const char *__sys_cgroup_memory_stat_inactive_anon_help =
    "Bytes of anonymous and swap cache memory on inactive LRU list";
const char *__sys_cgroup_memory_stat_active_anon_help =
    "Bytes of anonymous and swap cache memory on active LRU list";
const char *__sys_cgroup_memory_stat_inactive_file_help =
    "Bytes of file-backed memory on inactive LRU list";
const char *__sys_cgroup_memory_stat_active_file_help =
    "Bytes of file-backed memory on active LRU list";
const char *__sys_cgroup_memory_stat_unevictable_help =
    "Bytes of memory that cannot be reclaimed (mlocked, etc).";
const char *__sys_cgroup_memory_stat_hierarchical_memory_limit_help =
    "Bytes of memory limit with regard to hierarchy under which the memory cgroup is";
const char *__sys_cgroup_memory_stat_hierarchical_memsw_limit_help =
    "The memory+swap limit in place by the hierarchy cgroup";
const char *__sys_cgroup_memory_stat_stat_swap_help = "Bytes of swap memory used by the cgroup";

const char *__sys_cgroup_memory_stat_total_cache_help =
    "The equivalent of `cgroup.memory_stat_cache` that also includes the sum total of that metric "
    "for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_rss_help =
    "The equivalent of `cgroup.memory_stat_rss` that also includes the sum total of that metric "
    "for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_rss_huge_help =
    "The equivalent of `cgroup.memory_stat_rss_huge` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_shmem_help =
    "	The equivalent of `cgroup.memory_stat_shmem` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_swap_help =
    "Total amount of swap memory available to this cgroup";
const char *__sys_cgroup_memory_stat_total_mapped_file_help =
    "The equivalent of `cgroup.memory_stat_mapped_file` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_dirty_help =
    "	The equivalent of `cgroup.memory_stat_dirty` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_writeback_help =
    "The equivalent of `cgroup.memory_stat_writeback` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_pgpgin_help =
    "The equivalent of `cgroup.memory_stat_pgpgin` that also includes the sum total of that metric "
    "for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_pgpgout_help =
    "The equivalent of `cgroup.memory_stat_pgpgout` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_pgfault_help =
    "The equivalent of `cgroup.memory_stat_pgfault` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_pgmajfault_help =
    "he equivalent of `cgroup.memory_stat_pgmajfault` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_inactive_anon_help =
    "The equivalent of `cgroup.memory_stat_inactive_anon` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_active_anon_help =
    "The equivalent of `cgroup.memory_stat_active_anon` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_inactive_file_help =
    "The equivalent of `cgroup.memory_stat_inactive_file` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_active_file_help =
    "The equivalent of `cgroup.memory_stat_active_file` that also includes the sum total of that "
    "metric for all descendant cgroups";
const char *__sys_cgroup_memory_stat_total_unevictable_help =
    "The equivalent of `cgroup.memory_stat_unevictable` that also includes the sum total of that "
    "metric for all descendant cgroups";

const char *__sys_cgroup_memory_usage_in_bytes_help = "Bytes of memory used by the cgroup";
const char *__sys_cgroup_memory_limit_in_bytes_help =
    "The maximum amount of user memory (including file cache). A value of `9223372036854771712` "
    "(the max 64-bit int aligned to the nearest memory page) indicates no limit and is the "
    "default.";
const char *__sys_cgroup_memory_failcnt_help =
    "The number of times that the memory limit has reached the `limit_in_bytes` (reported in "
    "metric `cgroup.memory_limit_in_bytes`).";
const char *__sys_cgroup_memory_max_usage_in_bytes_help =
    "The maximum memory used by processes in the cgroup (in bytes)";
const char *__sys_cgroup_memory_swappiness_help = "The swappiness value for the cgroup";
