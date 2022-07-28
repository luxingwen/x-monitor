/*
 * @Author: CALM.WU
 * @Date: 2022-01-10 11:31:16
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-28 18:01:31
 */

// https://www.jianshu.com/p/aea52895de5e
// https://supportcenter.checkpoint.com/supportcenter/portal?eventSubmit_doGoviewsolutiondetails=&solutionid=sk65143
// https://www.cnblogs.com/liushui-sky/p/9236007.html
// https://man7.org/linux/man-pages/man5/proc.5.html

#include "plugin_proc.h"

#include "prometheus-client-c/prom.h"
#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"
#include "utils/procfile.h"
#include "utils/os.h"
#include "utils/strings.h"
#include "app_config/app_config.h"
#include "proc_rdset.h"

static const char       *__def_proc_stat_filename = "/proc/stat";
static const char       *__cfg_proc_stat_filename = NULL;
static struct proc_file *__pf_stat = NULL;

static prom_gauge_t *__metric_node_processes_running = NULL,
                    *__metric_node_processes_blocked = NULL, *__metric_node_cpus_count = NULL;

static prom_counter_t *__metric_node_cpu_jiffies = NULL,
                      *__metric_node_context_switches_from_boot = NULL,
                      *__metric_node_processes_from_boot = NULL,
                      *__metric_node_interrupts_from_boot = NULL;

int32_t init_collector_proc_stat() {
    // 设置prom指标
    if (unlikely(!__metric_node_interrupts_from_boot)) {
        __metric_node_interrupts_from_boot =
            prom_collector_registry_must_register_metric(prom_counter_new(
                "node_interrupts", "CPU counts of interrupts serviced since boot time ", 1,
                (const char *[]){ "cpu" }));
    }

    if (unlikely(!__metric_node_context_switches_from_boot)) {
        __metric_node_context_switches_from_boot = prom_collector_registry_must_register_metric(
            prom_counter_new("node_context_switches", "number of context switches across all CPUs",
                             1, (const char *[]){ "cpu" }));
    }

    if (unlikely(!__metric_node_processes_from_boot)) {
        __metric_node_processes_from_boot = prom_collector_registry_must_register_metric(
            prom_counter_new("node_procs_from_boot", "number of processes and threads created", 1,
                             (const char *[]){ "cpu" }));
    }

    if (unlikely(!__metric_node_processes_running)) {
        __metric_node_processes_running = prom_collector_registry_must_register_metric(
            prom_gauge_new("node_procs_running", "number of processes in runnable state", 1,
                           (const char *[]){ "cpu" }));
    }

    if (unlikely(!__metric_node_processes_blocked)) {
        __metric_node_processes_blocked = prom_collector_registry_must_register_metric(
            prom_gauge_new("node_procs_blocked",
                           "number of processes currently blocked, waiting for I/O to complete", 1,
                           (const char *[]){ "cpu" }));
    }

    if (unlikely(!__metric_node_cpu_jiffies)) {
        __metric_node_cpu_jiffies = prom_collector_registry_must_register_metric(
            prom_counter_new("node_cpu_jiffies", "jiffies the cpus spent in each mode", 2,
                             (const char *[]){ "cpu", "mode" }));
    }

    if (unlikely(!__metric_node_cpus_count)) {
        __metric_node_cpus_count = prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_cpus_count",
            " The count of CPUs per node, useful for getting CPU time as a percent of total.", 2,
            (const char *[]){ "cpu", "mode" }));
    }

    debug("[PLUGIN_PROC:proc_stat] init successed");
    return 0;
}

static void __do_cpu_utilization(size_t line, const char *cpu_label, bool is_core) {
    // debug("[PLUGIN_PROC:proc_stat] /proc/stat line: %lu, core_index: %d", line, core_index);

    // sysconf(_SC_CLK_TCK)一般地定义为jiffies(一般地等于10ms)
    // CPU时间 = user + system + nice + idle + iowait + irq + softirq

    uint64_t user_jiffies,   // 用户态时间
        nice_jiffies,        // nice用户态时间
        system_jiffies,      // 系统态时间
        idle_jiffies,        // 空闲时间, 不包含IO等待时间
        io_wait_jiffies,     // IO等待时间
        irq_jiffies,         // 硬中断时间
        soft_irq_jiffies,    // 软中断时间
        steal_jiffies,   // 虚拟化环境中运行其他操作系统上花费的时间（since Linux 2.6.11）
        guest_jiffies,        // 操作系统运行虚拟CPU花费的时间（since Linux 2.6.24）
        guest_nice_jiffies;   // 运行一个带nice值的guest花费的时间（since Linux 2.6.24）

    struct proc_cpu_rdset *cpu_rdset = NULL;

    if (likely(is_core)) {
        uint32_t cpu_id = str2uint32_t(cpu_label);
        cpu_rdset = &proc_rds->stat_rdset.core_rdsets[cpu_id];
    } else {
        cpu_rdset = &proc_rds->stat_rdset.cpu_rdset;
    }

    user_jiffies = str2uint64_t(procfile_lineword(__pf_stat, line, 1));
    nice_jiffies = str2uint64_t(procfile_lineword(__pf_stat, line, 2));
    system_jiffies = str2uint64_t(procfile_lineword(__pf_stat, line, 3));
    idle_jiffies = str2uint64_t(procfile_lineword(__pf_stat, line, 4));
    io_wait_jiffies = str2uint64_t(procfile_lineword(__pf_stat, line, 5));
    irq_jiffies = str2uint64_t(procfile_lineword(__pf_stat, line, 6));
    soft_irq_jiffies = str2uint64_t(procfile_lineword(__pf_stat, line, 7));
    steal_jiffies = str2uint64_t(procfile_lineword(__pf_stat, line, 8));
    guest_jiffies = str2uint64_t(procfile_lineword(__pf_stat, line, 9));
    guest_nice_jiffies = str2uint64_t(procfile_lineword(__pf_stat, line, 10));

    user_jiffies -= guest_jiffies;
    nice_jiffies -= guest_nice_jiffies;

    // 单位是jiffies
    prom_counter_add(__metric_node_cpu_jiffies, (double)(user_jiffies - cpu_rdset->user),
                     (const char *[]){ cpu_label, "user" });
    cpu_rdset->user = user_jiffies;

    prom_counter_add(__metric_node_cpu_jiffies, (double)(nice_jiffies - cpu_rdset->nice),
                     (const char *[]){ cpu_label, "nice" });
    cpu_rdset->nice = nice_jiffies;

    prom_counter_add(__metric_node_cpu_jiffies, (double)(system_jiffies - cpu_rdset->system),
                     (const char *[]){ cpu_label, "system" });
    cpu_rdset->system = system_jiffies;

    prom_counter_add(__metric_node_cpu_jiffies, (double)(idle_jiffies - cpu_rdset->idle),
                     (const char *[]){ cpu_label, "idle" });
    cpu_rdset->idle = idle_jiffies;

    prom_counter_add(__metric_node_cpu_jiffies, (double)(io_wait_jiffies - cpu_rdset->iowait),
                     (const char *[]){ cpu_label, "iowait" });
    cpu_rdset->iowait = io_wait_jiffies;

    prom_counter_add(__metric_node_cpu_jiffies, (double)(irq_jiffies - cpu_rdset->irq),
                     (const char *[]){ cpu_label, "irq" });
    cpu_rdset->irq = irq_jiffies;

    prom_counter_add(__metric_node_cpu_jiffies, (double)(soft_irq_jiffies - cpu_rdset->softirq),
                     (const char *[]){ cpu_label, "softirq" });
    cpu_rdset->softirq = soft_irq_jiffies;

    prom_counter_add(__metric_node_cpu_jiffies, (double)(steal_jiffies - cpu_rdset->steal),
                     (const char *[]){ cpu_label, "steal" });
    cpu_rdset->steal = steal_jiffies;

    prom_counter_add(__metric_node_cpu_jiffies, (double)(guest_jiffies - cpu_rdset->guest),
                     (const char *[]){ cpu_label, "guest" });
    cpu_rdset->guest = guest_jiffies;

    prom_counter_add(__metric_node_cpu_jiffies,
                     (double)(guest_nice_jiffies - cpu_rdset->guest_nice),
                     (const char *[]){ cpu_label, "guest_nice" });
    cpu_rdset->guest_nice = guest_nice_jiffies;

    debug("[PLUGIN_PROC:proc_stat] core_index: '%s' user_jiffies: %lu, nice_jiffies: %lu, "
          "system_jiffies: %lu, idle_jiffies: %lu, io_wait_jiffies: %lu, irq_jiffies: %lu, "
          "soft_irq_jiffies: %lu, steal_jiffies: %lu, guest_jiffies: %lu, guest_nice_jiffies: "
          "%lu",
          cpu_label, user_jiffies, nice_jiffies, system_jiffies, idle_jiffies, io_wait_jiffies,
          irq_jiffies, soft_irq_jiffies, steal_jiffies, guest_jiffies, guest_nice_jiffies);

    // 节点cpu时间总和，单位是jiffies
    uint64_t node_cpus_jiffies_total = 0;
    node_cpus_jiffies_total = user_jiffies + nice_jiffies + system_jiffies + idle_jiffies
                              + io_wait_jiffies + irq_jiffies + soft_irq_jiffies + steal_jiffies;
    prom_counter_add(__metric_node_cpu_jiffies,
                     (double)(node_cpus_jiffies_total - cpu_rdset->total),
                     (const char *[]){ cpu_label, "total" });
    cpu_rdset->total = node_cpus_jiffies_total;
}

int32_t collector_proc_stat(int32_t UNUSED(update_every), usec_t UNUSED(dt),
                            const char *config_path) {
    debug("[PLUGIN_PROC:proc_stat] config:%s running", config_path);

    if (unlikely(!__cfg_proc_stat_filename)) {
        __cfg_proc_stat_filename =
            appconfig_get_member_str(config_path, "monitor_file", __def_proc_stat_filename);
    }

    if (unlikely(!__pf_stat)) {
        __pf_stat = procfile_open(__cfg_proc_stat_filename, " \t:", PROCFILE_FLAG_DEFAULT);
        if (unlikely(!__pf_stat)) {
            error("[PLUGIN_PROC:proc_stat] Cannot open %s", __cfg_proc_stat_filename);
            return -1;
        }
        debug("[PLUGIN_PROC:proc_stat] opened '%s'", __cfg_proc_stat_filename);
    }

    __pf_stat = procfile_readall(__pf_stat);
    if (unlikely(!__pf_stat)) {
        error("Cannot read %s", __cfg_proc_stat_filename);
        return -1;
    }

    size_t lines = procfile_lines(__pf_stat);
    size_t line_words = 0;

    uint64_t interrupts_from_boot = 0;
    uint64_t context_switches_from_boot = 0;
    uint64_t processes_from_boot = 0;
    uint64_t processes_running = 0;
    uint64_t processes_blocked = 0;
    uint16_t cpus_count = 0;

    debug("[PLUGIN_PROC:proc_stat] lines: %lu", lines);

    for (size_t index = 0; index < lines; index++) {
        char *row_name = procfile_lineword(__pf_stat, index, 0);

        if (likely(row_name[0] == 'c' && row_name[1] == 'p' && row_name[2] == 'u')) {
            line_words = procfile_linewords(__pf_stat, index);

            if (unlikely(line_words < 10)) {
                error("Cannot read /proc/stat cpu line. Expected 10 params, read %lu.", line_words);
                continue;
            }

            bool is_core = (row_name[3] != '\0');
            if (!is_core) {
                __do_cpu_utilization(index, row_name, is_core);
            } else {
                __do_cpu_utilization(index, &row_name[3], is_core);
                cpus_count++;
            }
        } else if (unlikely(strncmp(row_name, "intr", 4) == 0)) {
            // “intr”这行给出中断的信息，第一个为自系统启动以来，发生的所有的中断的次数；然后每个数对应一个特定的中断自系统启动以来所发生的次数。
            // The first column is the total of all interrupts serviced; each subsequent column
            // is the total for that particular interrupt
            interrupts_from_boot = str2uint64_t(procfile_lineword(__pf_stat, index, 1));
            debug("[PLUGIN_PROC:proc_stat] interrupts_from_boot: %lu", interrupts_from_boot);

            prom_counter_add(
                __metric_node_interrupts_from_boot,
                (double)(interrupts_from_boot - proc_rds->stat_rdset.interrupts_from_boot),
                (const char *[]){ "intr" });
            proc_rds->stat_rdset.interrupts_from_boot = interrupts_from_boot;
        } else if (unlikely(strncmp(row_name, "ctxt", 4) == 0)) {
            // The "ctxt" line gives the total number of context switches across all CPUs.
            context_switches_from_boot = str2uint64_t(procfile_lineword(__pf_stat, index, 1));
            debug("[PLUGIN_PROC:proc_stat] context_switches_from_boot: %lu",
                  context_switches_from_boot);

            prom_counter_add(__metric_node_context_switches_from_boot,
                             (double)(context_switches_from_boot
                                      - proc_rds->stat_rdset.context_switches_from_boot),
                             (const char *[]){ "ctxt" });
            proc_rds->stat_rdset.context_switches_from_boot = context_switches_from_boot;
        } else if (unlikely(strncmp(row_name, "processes", 9) == 0)) {
            // The "processes" line gives the number of processes and threads created, which
            // includes (but is not limited to) those created by calls to the fork() and clone()
            // system calls.
            processes_from_boot = str2uint64_t(procfile_lineword(__pf_stat, index, 1));
            debug("[PLUGIN_PROC:proc_stat] processes_from_boot :%lu", processes_from_boot);

            prom_counter_add(
                __metric_node_processes_from_boot,
                (double)(processes_from_boot - proc_rds->stat_rdset.processes_from_boot),
                (const char *[]){ "processes" });
            proc_rds->stat_rdset.processes_from_boot = processes_from_boot;

        } else if (unlikely(strncmp(row_name, "procs_running", 13) == 0)) {
            // The "procs_running" line gives the number of processes currently running on CPUs.
            processes_running = str2uint64_t(procfile_lineword(__pf_stat, index, 1));
            debug("[PLUGIN_PROC:proc_stat] processes_running %lu", processes_running);
            prom_gauge_set(__metric_node_processes_running, (double)processes_running,
                           (const char *[]){ "procs_running" });

        } else if (unlikely(strncmp(row_name, "procs_blocked", 13) == 0)) {
            // The "procs_blocked" line gives the number of processes currently blocked, waiting
            // for I/O to complete.
            processes_blocked = str2uint64_t(procfile_lineword(__pf_stat, index, 1));
            debug("[PLUGIN_PROC:proc_stat] procs_blocked: %lu", processes_blocked);
            prom_gauge_set(__metric_node_processes_blocked, (double)processes_blocked,
                           (const char *[]){ "procs_blocked" });
        }

        prom_gauge_set(__metric_node_cpus_count, (double)cpus_count,
                       (const char *[]){ "core_count", "core_count" });
    }

    return 0;
}

void fini_collector_proc_stat() {
    if (likely(__pf_stat)) {
        procfile_close(__pf_stat);
        __pf_stat = NULL;
    }

    debug("[PLUGIN_PROC:proc_stat] stopped");
}
