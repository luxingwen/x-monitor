/*
 * @Author: CALM.WU
 * @Date: 2022-03-24 14:05:14
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-03-24 16:46:54
 */
#include "plugin_proc.h"
#include "proc_rdset.h"

#include "prometheus-client-c/prom.h"

#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"
#include "utils/procfile.h"
#include "utils/os.h"
#include "utils/strings.h"
#include "utils/clocks.h"

#include "app_config/app_config.h"

// https://www.kernel.org/doc/html/latest/scheduler/sched-stats.html
// https://www.kernel.org/doc/Documentation/scheduler/sched-stats.txt

static const char       *__def_proc_schedstat_filename = "/proc/schedstat";
static const char       *__cfg_proc_schedstat_filename = NULL;
static struct proc_file *__pf_schedstat = NULL;

static prom_counter_t *__metric_node_schedstat_running_seconds_total = NULL,
                      *__metric_node_schedstat_timeslices_total = NULL,
                      *__metric_node_schedstat_waiting_seconds_total = NULL;

int32_t init_collector_proc_schedstat() {
    __metric_node_schedstat_running_seconds_total =
        prom_collector_registry_must_register_metric(prom_counter_new(
            "node_schedstat_running_seconds_total",
            "Number of seconds CPU spent running a process.", 1, (const char *[]){ "cpu" }));
    __metric_node_schedstat_waiting_seconds_total = prom_collector_registry_must_register_metric(
        prom_counter_new("node_schedstat_waiting_seconds_total",
                         "Number of seconds spent by processing waiting for this CPU.", 1,
                         (const char *[]){ "cpu" }));
    __metric_node_schedstat_timeslices_total = prom_collector_registry_must_register_metric(
        prom_counter_new("node_schedstat_timeslices_total", "Number of timeslices executed by CPU.",
                         1, (const char *[]){ "cpu" }));

    debug("[PLUGIN_PROC:proc_schedstat] init successed");
    return 0;
}

int32_t collector_proc_schedstat(int32_t UNUSED(update_every), usec_t UNUSED(dt),
                                 const char *config_path) {
    debug("[PLUGIN_PROC:proc_schedstat] config:%s running", config_path);

    if (unlikely(!__cfg_proc_schedstat_filename)) {
        __cfg_proc_schedstat_filename =
            appconfig_get_member_str(config_path, "monitor_file", __def_proc_schedstat_filename);
    }

    if (unlikely(!__pf_schedstat)) {
        __pf_schedstat =
            procfile_open(__cfg_proc_schedstat_filename, " \t:", PROCFILE_FLAG_DEFAULT);
        if (unlikely(!__pf_schedstat)) {
            error("[PLUGIN_PROC:proc_net_proc_schedstatsockstat] Cannot open %s",
                  __cfg_proc_schedstat_filename);
            return -1;
        }
        debug("[PLUGIN_PROC:proc_net_proc_schedstatsockstat] opened '%s'",
              __cfg_proc_schedstat_filename);
    }

    __pf_schedstat = procfile_readall(__pf_schedstat);
    if (unlikely(!__pf_schedstat)) {
        error("Cannot read %s", __cfg_proc_schedstat_filename);
        return -1;
    }

    size_t lines = procfile_lines(__pf_schedstat);
    size_t words = 0;

    double   schedstat_running_seconds = 0.0, schedstat_waiting_seconds = 0.0;
    uint64_t schedstat_timeslices = 0;

    for (size_t l = 0; l < lines - 1; l++) {
        const char *row_key = procfile_lineword(__pf_schedstat, l, 0);

        // 判断是否是 cpu 的数据
        if (row_key[0] == 'c' && row_key[1] == 'p' && row_key[2] == 'u') {
            words = procfile_linewords(__pf_schedstat, l);
            if (unlikely(words < 10)) {
                error("Cannot read /proc/schedstat cpu line. Expected 9 params, read %lu.", words);
                continue;
            }

            const char *core_id = &row_key[3];
            uint32_t    cpu_id = str2uint32_t(core_id);

            // 文件的取值单位是纳秒，需要转换为秒，除以10的9次方
            // sum of all time spent running by tasks on this processor (in nanoseconds)
            schedstat_running_seconds =
                (double)str2uint64_t(procfile_lineword(__pf_schedstat, l, 7))
                / (double)NSEC_PER_SEC;

            // sum of all time spent waiting to run by tasks on this processor (in nanoseconds)
            schedstat_waiting_seconds =
                (double)str2uint64_t(procfile_lineword(__pf_schedstat, l, 8))
                / (double)NSEC_PER_SEC;
            // of timeslices run on this cpu 时间片
            schedstat_timeslices = str2uint64_t(procfile_lineword(__pf_schedstat, l, 9));

            debug("[PLUGIN_PROC:proc_schedstat] cpu:%s current running:%lf waiting:%lf "
                  "timeslices:%lu, prev running:%lf waiting:%lf timeslices:%lu",
                  core_id, schedstat_running_seconds, schedstat_waiting_seconds,
                  schedstat_timeslices, proc_rds->schedstats[cpu_id].schedstat_running_secs,
                  proc_rds->schedstats[cpu_id].schedstat_waiting_secs,
                  proc_rds->schedstats[cpu_id].schedstat_timeslices);

            prom_counter_add(__metric_node_schedstat_running_seconds_total,
                             (double)(schedstat_running_seconds
                                      - proc_rds->schedstats[cpu_id].schedstat_running_secs),
                             (const char *[]){ core_id });
            proc_rds->schedstats[cpu_id].schedstat_running_secs = schedstat_running_seconds;

            prom_counter_add(__metric_node_schedstat_waiting_seconds_total,
                             (double)(schedstat_waiting_seconds
                                      - proc_rds->schedstats[cpu_id].schedstat_waiting_secs),
                             (const char *[]){ core_id });
            proc_rds->schedstats[cpu_id].schedstat_waiting_secs = schedstat_waiting_seconds;

            prom_counter_add(
                __metric_node_schedstat_timeslices_total,
                (double)(schedstat_timeslices - proc_rds->schedstats[cpu_id].schedstat_timeslices),
                (const char *[]){ core_id });
            proc_rds->schedstats[cpu_id].schedstat_timeslices = schedstat_timeslices;
        }
    }

    return 0;
}

void fini_collector_proc_schedstat() {
    if (likely(__pf_schedstat)) {
        procfile_close(__pf_schedstat);
        __pf_schedstat = NULL;
    }

    debug("[PLUGIN_PROC:proc_schedstat] stopped");
}