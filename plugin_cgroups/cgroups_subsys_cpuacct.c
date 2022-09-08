/*
 * @Author: CALM.WU
 * @Date: 2022-09-07 11:06:02
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-07 16:48:09
 */

#include "cgroups_subsystem.h"
#include "cgroups_def.h"

#include "utils/common.h"
#include "utils/log.h"
#include "utils/compiler.h"
#include "utils/strings.h"
#include "utils/procfile.h"
#include "utils/strings.h"
#include "utils/files.h"
#include "utils/os.h"

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

    cg_obj->cg_metrics.cgroup_metric_cpuacct_stat_user_userhzs = prom_gauge_new(
        "cgroup.cpuacct_stat_user_userhzs", __sys_cgroup_metric_cpuacct_stat_user_userhzs_help, 1,
        (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_stat_user_userhzs);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_stat_system_userhzs = prom_gauge_new(
        "cgroup.cpuacct_stat_system_userhzs", __sys_cgroup_metric_cpuacct_stat_system_userhzs_help,
        1, (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_stat_system_userhzs);

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

    cg_obj->cg_counters.usage_per_cpus = (struct sys_cgroup_usage_per_cpu *)calloc(
        system_processor_num, sizeof(struct sys_cgroup_usage_per_cpu));

    debug("[PLUGIN_CGROUPS] init cgroup obj:'%s' subsys-cpuacct metrics success.", cg_obj->cg_id);
}

void read_cgroup_obj_cpuacct_metrics(struct xm_cgroup_obj *cg_obj) {
    debug("[PLUGIN_CGROUPS] read cgroup obj:'%s' subsys-cpuacct metrics", cg_obj->cg_id);

    static struct proc_file *pf = NULL;
    size_t                   index = 0;
    size_t                   words = 0;

    uint64_t user_userhzs = 0, system_userhzs = 0, nr_periods = 0, nr_throttled = 0,
             throttled_time_ns = 0, cpu_shares = 0, cfs_period_us = 0, cfs_quota_us = 0,
             usage_percpu_user_ns = 0, usage_percpu_sys_ns = 0, usage_percpu_ns = 0;

    if (likely(cg_obj->cpuacct_cpu_stat_filename)) {
        // read cpuacct.cpu.stat
        pf = procfile_reopen(pf, cg_obj->cpuacct_cpu_stat_filename, NULL, PROCFILE_FLAG_DEFAULT);
        if (likely(pf)) {
            // 读取文件
            pf = procfile_readall(pf);
            size_t lines = procfile_lines(pf);
            if (likely(lines >= 3)) {
                // 读取数值
                for (index = 0; index < lines; index++) {
                    char *key = procfile_lineword(pf, index, 0);
                    if (0 == strncmp(key, "nr_periods", 10)) {
                        nr_periods = str2uint64_t(procfile_lineword(pf, index, 1));
                    } else if (0 == strncmp(key, "nr_throttled", 12)) {
                        nr_throttled = str2uint64_t(procfile_lineword(pf, index, 1));
                    } else if (0 == strncmp(key, "throttled_time", 14)) {
                        throttled_time_ns = str2uint64_t(procfile_lineword(pf, index, 1));
                    }
                }
            } else {
                error("[PLUGIN_CGROUPS] cgroup file:'%s' should have 3 lines",
                      cg_obj->cpuacct_cpuacct_stat_filename);
            }
        } else {
            // don't continue
            return;
        }
    }

    if (likely(cg_obj->cpuacct_cpuacct_stat_filename)) {
        // read cpuacct.cpuacct.stat
        pf =
            procfile_reopen(pf, cg_obj->cpuacct_cpuacct_stat_filename, NULL, PROCFILE_FLAG_DEFAULT);
        if (likely(pf)) {
            // 读取文件
            pf = procfile_readall(pf);
            size_t lines = procfile_lines(pf);
            if (likely(lines >= 2)) {
                // 读取数值
                for (index = 0; index < lines; index++) {
                    char *key = procfile_lineword(pf, index, 0);
                    if (0 == strncmp(key, "user", 4)) {
                        user_userhzs = str2uint64_t(procfile_lineword(pf, index, 1));
                    } else if (0 == strncmp(key, "system", 6)) {
                        system_userhzs = str2uint64_t(procfile_lineword(pf, index, 1));
                    }
                }
            } else {
                error("[PLUGIN_CGROUPS] cgroup file:'%s' should have 2 lines",
                      cg_obj->cpuacct_cpuacct_stat_filename);
            }
        } else {
            // don't continue
            return;
        }
    }

    if (likely(cg_obj->cpuacct_cpu_shares_filename)) {
        if (unlikely(0 != read_file_to_uint64(cg_obj->cpuacct_cpu_shares_filename, &cpu_shares))) {
            // don't continue
            return;
        }
    }

    if (likely(cg_obj->cpuacct_cpu_cfs_period_us_filename)) {
        if (unlikely(0
                     != read_file_to_uint64(cg_obj->cpuacct_cpu_cfs_period_us_filename,
                                            &cfs_period_us))) {
            // don't continue
            return;
        }
    }

    if (likely(cg_obj->cpuacct_cpu_cfs_quota_us_filename)) {
        if (unlikely(
                0
                != read_file_to_uint64(cg_obj->cpuacct_cpu_cfs_quota_us_filename, &cfs_quota_us))) {
            // don't continue
            return;
        }
    }

    if (likely(cg_obj->cpuacct_usage_all_filename)) {
        // read cpuacct.cpuacct.usage_all
        pf = procfile_reopen(pf, cg_obj->cpuacct_usage_all_filename, NULL, PROCFILE_FLAG_DEFAULT);
        if (likely(pf)) {
            // 读取文件
            pf = procfile_readall(pf);
            size_t lines = procfile_lines(pf);
            if (likely(lines >= (system_processor_num + 1))) {
                // 读取数值
                for (index = 1; index < lines; index++) {
                    words = procfile_linewords(pf, index);
                    if (likely(words >= 3)) {
                        usage_percpu_user_ns = str2uint64_t(procfile_lineword(pf, index, 1));
                        usage_percpu_sys_ns = str2uint64_t(procfile_lineword(pf, index, 2));
                        usage_percpu_ns = usage_percpu_user_ns + usage_percpu_sys_ns;

                        // call prom_counter_add

                        cg_obj->cg_counters.usage_per_cpus[index - 1].usage_user_ns =
                            usage_percpu_user_ns;
                        cg_obj->cg_counters.usage_per_cpus[index - 1].usage_system_ns =
                            usage_percpu_sys_ns;
                        cg_obj->cg_counters.usage_per_cpus[index - 1].usage_ns = usage_percpu_ns;
                    }
                }
            } else {
                error("[PLUGIN_CGROUPS] cgroup file:'%s' should have 2 lines",
                      cg_obj->cpuacct_cpuacct_stat_filename);
            }
        } else {
            // don't continue
            return;
        }
    }

    if (likely(cg_obj->cpuacct_usage_filename)) {
    }

    if (likely(cg_obj->cpuacct_usage_user_filename)) {
    }

    if (likely(cg_obj->cpuacct_usage_sys_filename)) {
    }

    // update metrics
}