/*
 * @Author: CALM.WU
 * @Date: 2022-09-07 11:06:02
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-07 16:48:09
 */

#include "cgroups_subsystem.h"
#include "cgroups_def.h"
#include "cgroups_utils.h"

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
        "cgroup_cpu_shares", __sys_cgroup_metric_cpu_shares_help, 1, (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_shares);

    cg_obj->cg_metrics.cgroup_metric_cpu_cfs_period_us =
        prom_gauge_new("cgroup_cpu_cfs_period_us", __sys_cgroup_metric_cpu_cfs_period_us_help, 1,
                       (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_cfs_period_us);

    cg_obj->cg_metrics.cgroup_metric_cpu_cfs_quota_us =
        prom_gauge_new("cgroup_cpu_cfs_quota_us", __sys_cgroup_metric_cpu_cfs_quota_us_help, 1,
                       (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_cfs_quota_us);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_stat_user_userhzs = prom_gauge_new(
        "cgroup_cpuacct_stat_user_userhzs", __sys_cgroup_metric_cpuacct_stat_user_userhzs_help, 1,
        (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_stat_user_userhzs);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_stat_system_userhzs = prom_gauge_new(
        "cgroup_cpuacct_stat_system_userhzs", __sys_cgroup_metric_cpuacct_stat_system_userhzs_help,
        1, (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_stat_system_userhzs);

    cg_obj->cg_metrics.cgroup_metric_cpu_stat_nr_periods =
        prom_counter_new("cgroup_cpu_stat_nr_periods", __sys_cgroup_metric_cpu_stat_nr_periods_help,
                         1, (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_stat_nr_periods);

    cg_obj->cg_metrics.cgroup_metric_cpu_stat_nr_throttled = prom_counter_new(
        "cgroup_cpu_stat_nr_throttled", __sys_cgroup_metric_cpu_stat_nr_throttled_help, 1,
        (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_stat_nr_throttled);

    cg_obj->cg_metrics.cgroup_metric_cpu_stat_throttled_time_ns = prom_counter_new(
        "cgroup_cpu_stat_throttled_time_ns", __sys_cgroup_metric_cpu_stat_throttled_time_ns_help, 1,
        (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpu_stat_throttled_time_ns);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_ns =
        prom_counter_new("cgroup_cpuacct_usage_ns", __sys_cgroup_metric_cpuacct_usage_ns_help, 1,
                         (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_ns);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_ns_per_cpu = prom_counter_new(
        "cgroup_cpuacct_usage_ns_per_cpu", __sys_cgroup_metric_cpuacct_usage_ns_per_cpu_help, 2,
        (const char *[]){ "cgroup", "cpu" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_ns_per_cpu);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_user_ns = prom_counter_new(
        "cgroup_cpuacct_usage_user_ns", __sys_cgroup_metric_cpuacct_usage_user_ns_help, 1,
        (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_user_ns);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_user_ns_per_cpu =
        prom_counter_new("cgroup_cpuacct_usage_user_ns_per_cpu",
                         __sys_cgroup_metric_cpuacct_usage_user_ns_per_cpu_help, 2,
                         (const char *[]){ "cgroup", "cpu" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_user_ns_per_cpu);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_system_ns = prom_counter_new(
        "cgroup_cpuacct_usage_system_ns", __sys_cgroup_metric_cpuacct_usage_system_ns_help, 1,
        (const char *[]){ "cgroup" });
    prom_collector_add_metric(cg_obj->cg_prom_collector,
                              cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_system_ns);

    cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_system_ns_per_cpu =
        prom_counter_new("cgroup_cpuacct_usage_system_ns_per_cpu",
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
             usage_percpu_user_ns = 0, usage_percpu_sys_ns = 0, usage_percpu_ns = 0, usage_ns = 0,
             usage_user_ns = 0, usage_sys_ns = 0;

    // V1
    if (cg_type == CGROUPS_V1) {
        if (likely(cg_obj->cpuacct_cpu_stat_filename)) {
            // read cpuacct.cpu.stat
            pf =
                procfile_reopen(pf, cg_obj->cpuacct_cpu_stat_filename, NULL, PROCFILE_FLAG_DEFAULT);
            if (likely(pf)) {
                // 读取文件
                pf = procfile_readall(pf);
                size_t lines = procfile_lines(pf);
                if (likely(lines >= 3)) {
                    // read value from every line
                    for (index = 0; index < lines; index++) {
                        const char *key = procfile_lineword(pf, index, 0);
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

                prom_counter_add(cg_obj->cg_metrics.cgroup_metric_cpu_stat_nr_periods,
                                 (double)(nr_periods - cg_obj->cg_counters.cpu_stat_nr_periods),
                                 (const char *[]){ cg_obj->cg_id });
                prom_counter_add(cg_obj->cg_metrics.cgroup_metric_cpu_stat_nr_throttled,
                                 (double)(nr_throttled - cg_obj->cg_counters.cpu_stat_nr_throttled),
                                 (const char *[]){ cg_obj->cg_id });
                prom_counter_add(
                    cg_obj->cg_metrics.cgroup_metric_cpu_stat_throttled_time_ns,
                    (double)(throttled_time_ns - cg_obj->cg_counters.cpu_stat_throttled_time_ns),
                    (const char *[]){ cg_obj->cg_id });

                cg_obj->cg_counters.cpu_stat_nr_periods = nr_periods;
                cg_obj->cg_counters.cpu_stat_nr_throttled = nr_throttled;
                cg_obj->cg_counters.cpu_stat_throttled_time_ns = throttled_time_ns;

                debug("[PLUGIN_CGROUPS] cgroup obj:'%s' cpu.stat nr_periods:%lu, nr_throttled:%lu, "
                      "throttled_time_ns:%lu",
                      cg_obj->cg_id, nr_periods, nr_throttled, throttled_time_ns);
            } else {
                // don't continue
                return;
            }
        }

        if (likely(cg_obj->cpuacct_cpuacct_stat_filename)) {
            // read cpuacct.cpuacct.stat
            pf = procfile_reopen(pf, cg_obj->cpuacct_cpuacct_stat_filename, NULL,
                                 PROCFILE_FLAG_DEFAULT);
            if (likely(pf)) {
                // 读取文件
                pf = procfile_readall(pf);
                size_t lines = procfile_lines(pf);
                if (likely(lines >= 2)) {
                    // 读取数值
                    for (index = 0; index < lines; index++) {
                        const char *key = procfile_lineword(pf, index, 0);
                        if (0 == strncmp(key, "user", 4)) {
                            user_userhzs = str2uint64_t(procfile_lineword(pf, index, 1));
                        } else if (0 == strncmp(key, "system", 6)) {
                            system_userhzs = str2uint64_t(procfile_lineword(pf, index, 1));
                        }
                    }
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_cpuacct_stat_user_userhzs,
                                   (double)user_userhzs, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_cpuacct_stat_system_userhzs,
                                   (double)system_userhzs, (const char *[]){ cg_obj->cg_id });
                    debug("[PLUGIN_CGROUPS] cgroup obj:'%s' cpuacct.stat user:%lu system:%lu",
                          cg_obj->cg_id, user_userhzs, system_userhzs);
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
            if (unlikely(
                    0 != read_file_to_uint64(cg_obj->cpuacct_cpu_shares_filename, &cpu_shares))) {
                // don't continue
                return;
            }
            prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_cpu_shares, (double)cpu_shares,
                           (const char *[]){ cg_obj->cg_id });
        }

        if (likely(cg_obj->cpuacct_cpu_cfs_period_us_filename)) {
            if (unlikely(0
                         != read_file_to_uint64(cg_obj->cpuacct_cpu_cfs_period_us_filename,
                                                &cfs_period_us))) {
                // don't continue
                return;
            }
            prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_cpu_cfs_period_us,
                           (double)cfs_period_us, (const char *[]){ cg_obj->cg_id });
        }

        if (likely(cg_obj->cpuacct_cpu_cfs_quota_us_filename)) {
            if (unlikely(0
                         != read_file_to_uint64(cg_obj->cpuacct_cpu_cfs_quota_us_filename,
                                                &cfs_quota_us))) {
                // don't continue
                return;
            }
            prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_cpu_cfs_quota_us, (double)cfs_quota_us,
                           (const char *[]){ cg_obj->cg_id });
        }

        debug("[PLUGIN_CGROUPS] cgroup:'%s' cpu.shares:%lu, cpu.cfs_period_us:%lu, "
              "cpu.cfs_quota_us:%lu",
              cg_obj->cg_id, cpu_shares, cfs_period_us, cfs_quota_us);

        if (likely(cg_obj->cpuacct_usage_all_filename)) {
            // read cpuacct.cpuacct.usage_all
            pf = procfile_reopen(pf, cg_obj->cpuacct_usage_all_filename, NULL,
                                 PROCFILE_FLAG_DEFAULT);
            if (likely(pf)) {
                // 读取文件
                pf = procfile_readall(pf);
                size_t lines = procfile_lines(pf);
                if (likely(lines >= (system_processor_num + 1))) {
                    // 读取数值
                    for (index = 1; index < lines; index++) {
                        words = procfile_linewords(pf, index);
                        if (likely(words >= 3)) {
                            const char *cpu_id = procfile_lineword(pf, index, 0);
                            usage_percpu_user_ns = str2uint64_t(procfile_lineword(pf, index, 1));
                            usage_percpu_sys_ns = str2uint64_t(procfile_lineword(pf, index, 2));
                            usage_percpu_ns = usage_percpu_user_ns + usage_percpu_sys_ns;

                            // call prom_counter_add
                            prom_counter_add(
                                cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_ns_per_cpu,
                                (double)(usage_percpu_ns
                                         - cg_obj->cg_counters.usage_per_cpus[index - 1].usage_ns),
                                (const char *[]){ cg_obj->cg_id, cpu_id });
                            prom_counter_add(
                                cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_user_ns_per_cpu,
                                (double)(usage_percpu_user_ns
                                         - cg_obj->cg_counters.usage_per_cpus[index - 1]
                                               .usage_user_ns),
                                (const char *[]){ cg_obj->cg_id, cpu_id });
                            prom_counter_add(
                                cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_system_ns_per_cpu,
                                (double)(usage_percpu_sys_ns
                                         - cg_obj->cg_counters.usage_per_cpus[index - 1]
                                               .usage_system_ns),
                                (const char *[]){ cg_obj->cg_id, cpu_id });

                            cg_obj->cg_counters.usage_per_cpus[index - 1].usage_user_ns =
                                usage_percpu_user_ns;
                            cg_obj->cg_counters.usage_per_cpus[index - 1].usage_system_ns =
                                usage_percpu_sys_ns;
                            cg_obj->cg_counters.usage_per_cpus[index - 1].usage_ns =
                                usage_percpu_ns;

                            debug("[PLUGIN_CGROUPS] cgroup:'%s' cpu_id:'%s' usage_user_ns:%lu, "
                                  "usage_sys_ns:%lu usage_ns:%lu",
                                  cg_obj->cg_id, cpu_id, usage_percpu_user_ns, usage_percpu_sys_ns,
                                  usage_percpu_ns);
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
            if (unlikely(0 != read_file_to_uint64(cg_obj->cpuacct_usage_filename, &usage_ns))) {
                // don't continue
                return;
            }
            prom_counter_add(cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_ns,
                             (double)(usage_ns - cg_obj->cg_counters.cpuacct_usage_ns),
                             (const char *[]){ cg_obj->cg_id });
            cg_obj->cg_counters.cpuacct_usage_ns = usage_ns;
        }

        if (likely(cg_obj->cpuacct_usage_user_filename)) {
            if (unlikely(
                    0
                    != read_file_to_uint64(cg_obj->cpuacct_usage_user_filename, &usage_user_ns))) {
                // don't continue
                return;
            }
            prom_counter_add(cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_user_ns,
                             (double)(usage_user_ns - cg_obj->cg_counters.cpuacct_usage_user_ns),
                             (const char *[]){ cg_obj->cg_id });
            cg_obj->cg_counters.cpuacct_usage_user_ns = usage_user_ns;
        }

        if (likely(cg_obj->cpuacct_usage_sys_filename)) {
            if (unlikely(
                    0 != read_file_to_uint64(cg_obj->cpuacct_usage_sys_filename, &usage_sys_ns))) {
                // don't continue
                return;
            }
            prom_counter_add(cg_obj->cg_metrics.cgroup_metric_cpuacct_usage_system_ns,
                             (double)(usage_sys_ns - cg_obj->cg_counters.cpuacct_usage_system_ns),
                             (const char *[]){ cg_obj->cg_id });
            cg_obj->cg_counters.cpuacct_usage_system_ns = usage_sys_ns;
        }

        debug("[PLUGIN_CGROUPS] cgroup:'%s' cpuacct.usage:'%lu' cpuacct.usage_user:'%lu' "
              "cpuacct.usage_sys:'%lu'",
              cg_obj->cg_id, usage_ns, usage_user_ns, usage_sys_ns);
    }
}