/*
 * @Author: CALM.WU
 * @Date: 2022-09-07 11:06:02
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-07 14:32:27
 */

#include "cgroups_subsystem.h"
#include "cgroups_def.h"

#include "utils/common.h"
#include "utils/log.h"
#include "utils/compiler.h"
#include "utils/strings.h"

#include "prometheus-client-c/prom.h"

void init_cgroup_obj_cpuacct_metrics(struct xm_cgroup_obj *cg_obj) {
    // 构造指标，加入collector
    cg_obj->cg_metrics.cgroup_metric_cpu_shares = prom_gauge_new(
        "cgroup.cpu_shares", __sys_cgroup_metric_cpu_shares_help, 1, (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_shares);

    cg_obj->cg_metrics.cgroup_metric_cpu_cfs_period_us =
        prom_gauge_new("cgroup.cpu_cfs_period_us", __sys_cgroup_metric_cpu_cfs_period_us_help, 1,
                       (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_cfs_period_us);

    cg_obj->cg_metrics.cgroup_metric_cpu_cfs_quota_us =
        prom_gauge_new("cgroup.cpu_cfs_quota_us", __sys_cgroup_metric_cpu_cfs_quota_us_help, 1,
                       (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_cfs_quota_us);

    cg_obj->cg_metrics.cgroup_metric_cpu_stat_nr_periods =
        prom_counter_new("cgroup.cpu_stat_nr_periods", __sys_cgroup_metric_cpu_stat_nr_periods_help,
                         1, (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_stat_nr_periods);

    cg_obj->cg_metrics.cgroup_metric_cpu_stat_nr_throttled = prom_counter_new(
        "cgroup.cpu_stat_nr_throttled", __sys_cgroup_metric_cpu_stat_nr_throttled_help, 1,
        (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_stat_nr_throttled);

    cg_obj->cg_metrics.cgroup_metric_cpu_stat_throttled_time_ns = prom_counter_new(
        "cgroup.cpu_stat_throttled_time_ns", __sys_cgroup_metric_cpu_stat_throttled_time_ns_help, 1,
        (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_stat_throttled_time_ns);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_ns =
        prom_counter_new("cgroup.cpuacct_usage_ns", __sys_cgroup_metric_cpuacct_usage_ns_help, 1,
                         (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_ns);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_ns_per_cpu = prom_counter_new(
        "cgroup.cpuacct_usage_ns_per_cpu", __sys_cgroup_metric_cpuacct_usage_ns_per_cpu_help, 2,
        (const char *[]){ "cgroup", "cpu" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_ns_per_cpu);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_user_ns = prom_counter_new(
        "cgroup.cpuacct_usage_user_ns", __sys_cgroup_metric_cpuacct_usage_user_ns_help, 1,
        (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_user_ns);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_user_ns_per_cpu =
        prom_counter_new("cgroup.cpuacct_usage_user_ns_per_cpu",
                         __sys_cgroup_metric_cpuacct_usage_user_ns_per_cpu_help, 2,
                         (const char *[]){ "cgroup", "cpu" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_user_ns_per_cpu);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_system_ns = prom_counter_new(
        "cgroup.cpuacct_usage_system_ns", __sys_cgroup_metric_cpuacct_usage_system_ns_help, 1,
        (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_system_ns);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_system_ns_per_cpu =
        prom_counter_new("cgroup.cpuacct_usage_system_ns_per_cpu",
                         __sys_cgroup_metric_cpuacct_usage_system_ns_per_cpu_help, 2,
                         (const char *[]){ "cgroup", "cpu" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_system_ns_per_cpu);

    debug("[PLUGIN_CGROUPS] init cgroup obj:'%s' subsys-cpuacct metrics success.", cg_obj->cg_id);
}

void read_cgroup_obj_cpuacct_metrics(struct xm_cgroup_obj *cg_obj) {
    debug("[PLUGIN_CGROUPS] read cgroup obj:'%s' subsys-cpuacct metrics", cg_obj->cg_id);
}