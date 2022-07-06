/*
 * @Author: CALM.WU
 * @Date: 2022-01-10 10:49:20
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-03-24 14:10:34
 */

#include "plugin_proc.h"

#include "prometheus-client-c/prom.h"

#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"
#include "utils/procfile.h"
#include "utils/strings.h"

#include "app_config/app_config.h"

// http://brytonlee.github.io/blog/2014/05/07/linux-kernel-load-average-calc/

static const char       *__def_proc_loadavg_filename = "/proc/loadavg";
static const char       *__cfg_proc_loadavg_filename = NULL;
static struct proc_file *__pf_loadavg = NULL;

static prom_gauge_t *__metric_loadavg_1min = NULL, *__metric_loadavg_5min = NULL,
                    *__metric_loadavg_15min = NULL, *__metric_active_processes = NULL;

int32_t init_collector_proc_loadavg() {
    if (unlikely(!__metric_loadavg_1min)) {
        __metric_loadavg_1min = prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_load1", "System 1m Load Average", 1, (const char *[]){ "loadavg" }));
    }

    if (unlikely(!__metric_loadavg_5min)) {
        __metric_loadavg_5min = prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_load5", "System 5m Load Average", 1, (const char *[]){ "loadavg" }));
    }

    if (unlikely(!__metric_loadavg_15min)) {
        __metric_loadavg_15min = prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_load15", "System 15m Load Average", 1, (const char *[]){ "loadavg" }));
    }

    if (unlikely(!__metric_active_processes)) {
        __metric_active_processes = prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_forks_total", "Total number of forks", 1, (const char *[]){ "system" }));
    }
    debug("[PLUGIN_PROC:proc_loadavg] init successed");
    return 0;
}

int32_t collector_proc_loadavg(int32_t UNUSED(update_every), usec_t UNUSED(dt),
                               const char *config_path) {
    debug("[PLUGIN_PROC:proc_loadavg] config:%s running", config_path);

    if (unlikely(!__cfg_proc_loadavg_filename)) {
        __cfg_proc_loadavg_filename =
            appconfig_get_member_str(config_path, "monitor_file", __def_proc_loadavg_filename);
    }

    if (unlikely(!__pf_loadavg)) {
        __pf_loadavg = procfile_open(__cfg_proc_loadavg_filename, " \t,:|/", PROCFILE_FLAG_DEFAULT);
        if (unlikely(!__pf_loadavg)) {
            error("[PLUGIN_PROC:proc_loadavg] Cannot open %s", __cfg_proc_loadavg_filename);
            return -1;
        }
        debug("[PLUGIN_PROC:proc_loadavg] opened '%s'", __cfg_proc_loadavg_filename);
    }

    __pf_loadavg = procfile_readall(__pf_loadavg);
    if (unlikely(!__pf_loadavg)) {
        error("Cannot read %s", __cfg_proc_loadavg_filename);
        return -1;
    }

    if (unlikely(procfile_lines(__pf_loadavg) < 1)) {
        error("%s: File does not contain enough lines.", __cfg_proc_loadavg_filename);
        return -1;
    }

    if (unlikely(procfile_linewords(__pf_loadavg, 0) < 6)) {
        error("%s: File does not contain enough columns.", __cfg_proc_loadavg_filename);
        return -1;
    }

    double load_1m = strtod(procfile_lineword(__pf_loadavg, 0, 0), NULL);
    double load_5m = strtod(procfile_lineword(__pf_loadavg, 0, 1), NULL);
    double load_15m = strtod(procfile_lineword(__pf_loadavg, 0, 2), NULL);

    // /proc/stat中有该数值
    // uint64_t running_processes = str2ull(procfile_lineword(__pf_loadavg, 0, 3));
    uint64_t active_processes = str2ull(procfile_lineword(__pf_loadavg, 0, 4));
    pid_t    last_running_pid = (pid_t)strtoll(procfile_lineword(__pf_loadavg, 0, 5), NULL, 10);

    debug("[PLUGIN_PROC:proc_loadavg] LOAD AVERAGE: %.2f %.2f %.2f active_processes: %lu "
          "last_running_pid: %d",
          load_1m, load_5m, load_15m, active_processes, last_running_pid);

    prom_gauge_set(__metric_loadavg_1min, load_1m, (const char *[]){ "load" });

    prom_gauge_set(__metric_loadavg_5min, load_5m, (const char *[]){ "load" });

    prom_gauge_set(__metric_loadavg_15min, load_15m, (const char *[]){ "load" });

    prom_gauge_set(__metric_active_processes, active_processes, (const char *[]){ "load" });

    return 0;
}

void fini_collector_proc_loadavg() {
    if (likely(__pf_loadavg)) {
        procfile_close(__pf_loadavg);
        __pf_loadavg = NULL;
    }

    // if (likely(__metric_loadavg_1min)) {
    //     prom_gauge_destroy(__metric_loadavg_1min);
    //     __metric_loadavg_1min = NULL;
    // }

    // if (likely(__metric_loadavg_5min)) {
    //     prom_gauge_destroy(__metric_loadavg_5min);
    //     __metric_loadavg_5min = NULL;
    // }

    // if (likely(__metric_loadavg_15min)) {
    //     prom_gauge_destroy(__metric_loadavg_15min);
    //     __metric_loadavg_15min = NULL;
    // }

    // if (likely(__metric_active_processes)) {
    //     prom_gauge_destroy(__metric_active_processes);
    //     __metric_active_processes = NULL;
    // }

    debug("[PLUGIN_PROC:proc_loadavg] stopped");
}