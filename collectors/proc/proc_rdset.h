/*
 * @Author: CALM.WU
 * @Date: 2022-07-28 14:32:35
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-28 17:34:05
 */

#pragma

#include <stdint.h>
//#include <stdatomic.h>
//#include <stdbool.h>
#include "utils/compiler.h"

struct proc_cpu_rdset {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
    uint64_t guest;
    uint64_t guest_nice;
    uint64_t total;
};

struct proc_schedstat {
    double   schedstat_running_secs;
    double   schedstat_waiting_secs;
    uint64_t schedstat_timeslices;
};

struct proc_stat_rdset {
    struct proc_cpu_rdset *core_rdsets;
    struct proc_cpu_rdset  cpu_rdset;
    uint16_t               cpu_count;
    uint64_t               interrupts_from_boot;
    uint64_t               context_switches_from_boot;
    uint64_t               processes_from_boot;
    uint64_t               processes_running;   // is gauge type
    uint64_t               processes_blocked;   // is gauge type
};

struct proc_rdset {
    struct proc_stat_rdset stat_rdset;
    struct proc_schedstat *schedstats;
};

extern struct proc_rdset *proc_rds;

extern int32_t init_proc_rdset();

extern void fini_proc_rdset();
