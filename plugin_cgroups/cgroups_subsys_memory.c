/*
 * @Author: CALM.WU
 * @Date: 2022-09-07 11:36:44
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-07 14:33:57
 */

#include "cgroups_subsystem.h"
#include "cgroups_def.h"
#include "cgroups_utils.h"

#include "utils/common.h"
#include "utils/log.h"
#include "utils/compiler.h"
#include "utils/strings.h"
#include "utils/procfile.h"
#include "utils/adaptive_resortable_list.h"
#include "utils/files.h"

#include "prometheus-client-c/prom.h"

#define __register_new_cgroup_metric(metric, new_func, metric_name, collector, help) \
    do {                                                                             \
        metric = new_func(metric_name, help, 1, (const char *[]){ "cgroup" });       \
        prom_collector_add_metric(collector, metric);                                \
    } while (0)

static const char *const __vmpressure_str_levels[] = {
    [VMPRESSURE_LOW] = "low",
    [VMPRESSURE_MEDIUM] = "medium",
    [VMPRESSURE_CRITICAL] = "critical",
};

static uint64_t __cache_bytes = 0, __rss_bytes = 0, __rss_huge_bytes = 0, __shmem_bytes = 0,
                __mapped_file_bytes = 0, __dirty_bytes = 0, __writeback_bytes = 0, __pgpgin = 0,
                __pgpgout = 0, __pgfault = 0, __pgmajfault = 0, __inactive_anon_bytes = 0,
                __active_anon_bytes = 0, __inactive_file_bytes = 0, __active_file_bytes = 0,
                __unevictable_bytes = 0, __hierarchical_memory_limit_bytes = 0,
                __hierarchical_memsw_limit_bytes = 0, __swap_bytes = 0, __total_cache_bytes = 0,
                __total_rss_bytes = 0, __total_rss_huge_bytes = 0, __total_shmem_bytes = 0,
                __total_mapped_file_bytes = 0, __total_dirty_bytes = 0, __total_writeback_bytes = 0,
                __total_pgpgin = 0, __total_pgpgout = 0, __total_pgfault = 0,
                __total_pgmajfault = 0, __total_inactive_anon_bytes = 0,
                __total_active_anon_bytes = 0, __total_inactive_file_bytes = 0,
                __total_active_file_bytes = 0, __total_unevictable_bytes = 0;

void init_cgroup_obj_memory_metrics(struct xm_cgroup_obj *cg_obj) {
    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_cache_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_cache",
                                 cg_obj->cg_prom_collector, __sys_cgroup_memory_stat_cache_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_rss_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_rss",
                                 cg_obj->cg_prom_collector, __sys_cgroup_memory_stat_rss_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_rss_huge_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_rss_huge",
                                 cg_obj->cg_prom_collector, __sys_cgroup_memory_stat_rss_huge_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_shmem_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_shmem",
                                 cg_obj->cg_prom_collector, __sys_cgroup_memory_stat_shmem_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_mapped_file_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_mapped_file",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_mapped_file_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_dirty_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_dirty",
                                 cg_obj->cg_prom_collector, __sys_cgroup_memory_stat_dirty_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_writeback_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_writeback",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_writeback_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_pgpgin,
                                 prom_counter_new, "cgroup_memory_stat_pgpgin",
                                 cg_obj->cg_prom_collector, __sys_cgroup_memory_stat_pgpgin_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_pgpgout,
                                 prom_counter_new, "cgroup_memory_stat_pgpgout",
                                 cg_obj->cg_prom_collector, __sys_cgroup_memory_stat_pgpgout_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_pgfault,
                                 prom_counter_new, "cgroup_memory_stat_pgfault",
                                 cg_obj->cg_prom_collector, __sys_cgroup_memory_stat_pgfault_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_pgmajfault,
                                 prom_counter_new, "cgroup_memory_stat_pgmajfault",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_pgmajfault_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_inactive_anon_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_inactive_anon",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_inactive_anon_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_active_anon_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_active_anon",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_active_anon_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_inactive_file_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_inactive_file",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_inactive_file_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_active_file_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_active_file",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_active_file_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_unevictable_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_unevictable",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_unevictable_help);

    __register_new_cgroup_metric(
        cg_obj->cg_metrics.cgroup_metric_memory_stat_hierarchical_memory_limit_bytes,
        prom_gauge_new, "cgroup_memory_stat_hierarchical_memory_limit", cg_obj->cg_prom_collector,
        __sys_cgroup_memory_stat_hierarchical_memory_limit_help);

    __register_new_cgroup_metric(
        cg_obj->cg_metrics.cgroup_metric_memory_stat_hierarchical_memsw_limit_bytes, prom_gauge_new,
        "cgroup_memory_stat_hierarchical_memsw_limit", cg_obj->cg_prom_collector,
        __sys_cgroup_memory_stat_hierarchical_memsw_limit_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_swap_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_swap",
                                 cg_obj->cg_prom_collector, __sys_cgroup_memory_stat_swap_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_cache_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_total_cache",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_total_cache_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_rss_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_total_rss",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_total_rss_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_rss_huge_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_total_rss_huge",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_total_rss_huge_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_shmem_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_total_shmem",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_total_shmem_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_swap_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_total_swap",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_total_swap_help);

    __register_new_cgroup_metric(
        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_mapped_file_bytes, prom_gauge_new,
        "cgroup_memory_stat_total_mapped_file", cg_obj->cg_prom_collector,
        __sys_cgroup_memory_stat_total_mapped_file_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_dirty_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_total_dirty",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_total_dirty_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_writeback_bytes,
                                 prom_gauge_new, "cgroup_memory_stat_total_writeback",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_total_writeback_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_pgpgin,
                                 prom_counter_new, "cgroup_memory_stat_total_pgpgin",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_total_pgpgin_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_pgpgout,
                                 prom_counter_new, "cgroup_memory_stat_total_pgpgout",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_total_pgpgout_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_pgfault,
                                 prom_counter_new, "cgroup_memory_stat_total_pgfault",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_total_pgfault_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_pgmajfault,
                                 prom_counter_new, "cgroup_memory_stat_total_pgmajfault",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_stat_total_pgmajfault_help);

    __register_new_cgroup_metric(
        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_inactive_anon_bytes, prom_gauge_new,
        "cgroup_memory_stat_total_inactive_anon", cg_obj->cg_prom_collector,
        __sys_cgroup_memory_stat_total_inactive_anon_help);

    __register_new_cgroup_metric(
        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_active_anon_bytes, prom_gauge_new,
        "cgroup_memory_stat_total_active_anon", cg_obj->cg_prom_collector,
        __sys_cgroup_memory_stat_total_active_anon_help);

    __register_new_cgroup_metric(
        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_inactive_file_bytes, prom_gauge_new,
        "cgroup_memory_stat_total_inactive_file", cg_obj->cg_prom_collector,
        __sys_cgroup_memory_stat_total_inactive_file_help);

    __register_new_cgroup_metric(
        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_active_file_bytes, prom_gauge_new,
        "cgroup_memory_stat_total_active_file", cg_obj->cg_prom_collector,
        __sys_cgroup_memory_stat_total_active_file_help);

    __register_new_cgroup_metric(
        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_unevictable_bytes, prom_gauge_new,
        "cgroup_memory_stat_total_unevictable", cg_obj->cg_prom_collector,
        __sys_cgroup_memory_stat_total_unevictable_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_usage_in_bytes,
                                 prom_gauge_new, "cgroup_memory_usage_in_bytes",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_usage_in_bytes_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_limit_in_bytes,
                                 prom_gauge_new, "cgroup_memory_limit_in_bytes",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_limit_in_bytes_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_failcnt, prom_gauge_new,
                                 "cgroup_memory_failcnt", cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_failcnt_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_max_usage_in_bytes,
                                 prom_gauge_new, "cgroup_memory_max_usage_in_bytes",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_max_usage_in_bytes_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_swappiness, prom_gauge_new,
                                 "cgroup_memory_swappiness", cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_swappiness_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_pressure_level,
                                 prom_gauge_new, "cgroup_memory_pressure_level",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_pressure_level_help);

    arl_expect(cg_obj->arl_base_mem_stat, "cache", &__cache_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "rss", &__rss_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "rss_huge", &__rss_huge_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "mapped_file", &__mapped_file_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "dirty", &__dirty_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "writeback", &__writeback_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "pgpgin", &__pgpgin);
    arl_expect(cg_obj->arl_base_mem_stat, "pgpgout", &__pgpgout);
    arl_expect(cg_obj->arl_base_mem_stat, "pgfault", &__pgfault);
    arl_expect(cg_obj->arl_base_mem_stat, "pgmajfault", &__pgmajfault);
    arl_expect(cg_obj->arl_base_mem_stat, "inactive_anon", &__inactive_anon_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "active_anon", &__active_anon_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "inactive_file", &__inactive_file_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "active_file", &__active_file_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "unevictable", &__unevictable_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "hierarchical_memory_limit",
               &__hierarchical_memory_limit_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_cache", &__total_cache_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_rss", &__total_rss_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_rss_huge", &__total_rss_huge_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_shmem", &__total_shmem_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_mapped_file", &__total_mapped_file_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_dirty", &__total_dirty_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_writeback", &__total_writeback_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_pgpgin", &__total_pgpgin);
    arl_expect(cg_obj->arl_base_mem_stat, "total_pgpgout", &__total_pgpgout);
    arl_expect(cg_obj->arl_base_mem_stat, "total_pgfault", &__total_pgfault);
    arl_expect(cg_obj->arl_base_mem_stat, "total_pgmajfault", &__total_pgmajfault);
    arl_expect(cg_obj->arl_base_mem_stat, "total_inactive_anon", &__total_inactive_anon_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_active_anon", &__total_active_anon_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_inactive_file", &__total_inactive_file_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_active_file", &__total_active_file_bytes);
    arl_expect(cg_obj->arl_base_mem_stat, "total_unevictable", &__total_unevictable_bytes);

    debug("[PLUGIN_CGROUPS] init cgroup obj:'%s' subsys-memory metrics success.", cg_obj->cg_id);
}

int32_t init_cgroup_memory_pressure_listener(struct xm_cgroup_obj *cg_obj,
                                             const char           *cg_memory_base_path,
                                             const char           *mem_pressure_level_str) {
    char    file_path[PATH_MAX] = { 0 };
    char    line[LINE_MAX] = { 0 };
    int32_t evt_fd = -1, mem_pressure_fd = -1, ctrl_fd = -1;

    if (unlikely(!cg_obj)) {
        return -1;
    }

    if (cg_type == CGROUPS_V1) {
        // 设置了EFD_SEMAPHORE，每次读counter只会减1，这样可以保证level的时间更为持久
        if (0 == strcmp("critical", mem_pressure_level_str)
            || 0 == strcmp("medium", mem_pressure_level_str)) {
            evt_fd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
        } else {
            // 因为low数值会很多，这样每次read减1需要很久，所以一次读取
            evt_fd = eventfd(0, EFD_NONBLOCK);
        }

        if (unlikely(-1 == evt_fd)) {
            error("[PLUGIN_CGROUPS] create cgroup obj:'%s' memory eventfd failed, errno:%d, "
                  "error:%s.",
                  cg_obj->cg_id, errno, strerror(errno));
            return -1;
        }

        snprintf(file_path, sizeof(file_path), "%s/%s/memory.pressure_level", cg_memory_base_path,
                 cg_obj->cg_id);
        mem_pressure_fd = open(file_path, O_RDONLY);
        if (unlikely(-1 == mem_pressure_fd)) {
            error("[PLUGIN_CGROUPS] open cgroup obj:'%s' memory.pressure_level file failed, "
                  "errno:%d, error:%s.",
                  cg_obj->cg_id, errno, strerror(errno));
            goto failed_clean;
        } else {
            debug("[PLUGIN_CGROUPS] open cgroup obj:'%s' memory.pressure_level file success.",
                  cg_obj->cg_id);
        }

        snprintf(file_path, sizeof(file_path), "%s/%s/cgroup.event_control", cg_memory_base_path,
                 cg_obj->cg_id);
        ctrl_fd = open(file_path, O_WRONLY);
        if (unlikely(-1 == ctrl_fd)) {
            error("[PLUGIN_CGROUPS] open cgroup obj:'%s' cgroup.event_control file failed, "
                  "errno:%d, error:%s.",
                  cg_obj->cg_id, errno, strerror(errno));
            goto failed_clean;
        } else {
            debug("[PLUGIN_CGROUPS] open cgroup obj:'%s' cgroup.event_control file success.",
                  cg_obj->cg_id);
        }

        int32_t line_size;
        snprintf(line, sizeof(line), "%d %d %s%n", evt_fd, mem_pressure_fd, mem_pressure_level_str,
                 &line_size);
        if (unlikely(write(ctrl_fd, line, line_size) < 0)) {
            error("[PLUGIN_CGROUPS] write data:'%s' to cgroup obj:'%s'.cgroup.event_control file "
                  "failed, errno:%d, error:%s.",
                  line, cg_obj->cg_id, errno, strerror(errno));
            goto failed_clean;
        } else {
            debug("[PLUGIN_CGROUPS] write data:'%s' to cgroup obj:'%s'.cgroup.event_control file "
                  "success.",
                  line, cg_obj->cg_id);
        }
        close(mem_pressure_fd);
        close(ctrl_fd);
    }

    return evt_fd;

failed_clean:
    if (-1 != evt_fd) {
        close(evt_fd);
    }

    if (-1 != mem_pressure_fd) {
        close(mem_pressure_fd);
    }

    if (-1 != ctrl_fd) {
        close(ctrl_fd);
    }
    return -1;
}

static void __collect_cgroup_v1_memory_metrics(struct xm_cgroup_obj *cg_obj) {
    int32_t  ret = 0;
    uint32_t mem_pressure = 0;
    uint64_t counter_val = 0, usage_in_bytes = 0, limit_in_bytes = 0, failcnt = 0,
             max_usage_in_bytes = 0;

    // 会调用内核函数eventfd_ctx_do_read读取counter值，如果没有EFD_SEMAPHORE标志，则会将counter清零，否则counter只会减1
    // 内核vmpressure_event函数会判断pressure
    // level是否达到设定的level，如果达到则会调用eventfd_signal函数将counter加1，vmpressure.c

    if (likely(-1 != cg_obj->mem_pressure_low_level_evt_fd
               && -1 != cg_obj->mem_pressure_medium_level_evt_fd
               && -1 != cg_obj->mem_pressure_critical_level_evt_fd)) {
        ret = read(cg_obj->mem_pressure_critical_level_evt_fd, &counter_val, sizeof(counter_val));
        if (sizeof(counter_val) == ret) {
            warn("[PLUGIN_CGROUPS] the cgroup obj:'%s' memory pressure reached 'critical' "
                 "level, counter:%lu",
                 cg_obj->cg_id, counter_val);
            mem_pressure = 7;
        } else {
            ret = read(cg_obj->mem_pressure_medium_level_evt_fd, &counter_val, sizeof(counter_val));
            if (sizeof(counter_val) == ret) {
                warn("[PLUGIN_CGROUPS] the cgroup obj:'%s' memory pressure reached 'medium' "
                     "level, counter:%lu",
                     cg_obj->cg_id, counter_val);
                mem_pressure = 3;
            } else {
                ret =
                    read(cg_obj->mem_pressure_low_level_evt_fd, &counter_val, sizeof(counter_val));
                if (sizeof(counter_val) == ret) {
                    warn("[PLUGIN_CGROUPS] the cgroup obj:'%s' memory pressure reached 'low' "
                         "level, counter: %lu",
                         cg_obj->cg_id, counter_val);
                    mem_pressure = 1;
                }
            }
        }

        // 如果内存压力打到了critical level，那么就会将mem_pressure置为7
        // medium level, mem_pressure = 3
        // low level, mem_pressure = 1
        debug("[PLUGIN_CGROUPS] the cgroup:'%s' memory pressure: %d", cg_obj->cg_id, mem_pressure);
        prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_pressure_level, (double)mem_pressure,
                       (const char *[]){ cg_obj->cg_id });
    }

    // read memory.stat
    static struct proc_file *pf = NULL;
    if (likely(cg_obj->memory_stat_filename)) {
        // open file
        pf = procfile_reopen(pf, cg_obj->memory_stat_filename, NULL, PROCFILE_FLAG_DEFAULT);
        if (likely(pf)) {
            // read all file
            pf = procfile_readall(pf);
            if (likely(pf)) {
                size_t line_count = procfile_lines(pf);
                if (unlikely(line_count < 1)) {
                    error("[PLUGIN_CGROUPS] the cgroup obj:'%s' memory.stat file is empty.",
                          cg_obj->cg_id);
                } else {
                    // get line value
                    arl_begin(cg_obj->arl_base_mem_stat);
                    for (size_t l = 0; l < line_count; l++) {
                        size_t words = procfile_linewords(pf, l);
                        if (unlikely(words < 2))
                            continue;

                        if (unlikely(arl_check(cg_obj->arl_base_mem_stat,
                                               procfile_lineword(pf, l, 0),
                                               procfile_lineword(pf, l, 1)))) {
                            break;
                        }
                    }

                    // set metric
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_cache_bytes,
                                   (double)__cache_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_rss_bytes,
                                   (double)__rss_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_rss_huge_bytes,
                                   (double)__rss_huge_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_mapped_file_bytes,
                                   (double)__mapped_file_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_dirty_bytes,
                                   (double)__dirty_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_writeback_bytes,
                                   (double)__writeback_bytes, (const char *[]){ cg_obj->cg_id });

                    prom_counter_add(cg_obj->cg_metrics.cgroup_metric_memory_stat_pgpgin,
                                     (double)(__pgpgin - cg_obj->cg_counters.memory_stat_pgpgin),
                                     (const char *[]){ cg_obj->cg_id });
                    cg_obj->cg_counters.memory_stat_pgpgin = __pgpgin;

                    prom_counter_add(cg_obj->cg_metrics.cgroup_metric_memory_stat_pgpgout,
                                     (double)(__pgpgout - cg_obj->cg_counters.memory_stat_pgpgout),
                                     (const char *[]){ cg_obj->cg_id });
                    cg_obj->cg_counters.memory_stat_pgpgout = __pgpgout;

                    prom_counter_add(cg_obj->cg_metrics.cgroup_metric_memory_stat_pgfault,
                                     (double)(__pgfault - cg_obj->cg_counters.memory_stat_pgfault),
                                     (const char *[]){ cg_obj->cg_id });
                    cg_obj->cg_counters.memory_stat_pgfault = __pgfault;

                    prom_counter_add(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_pgmajfault,
                        (double)(__pgmajfault - cg_obj->cg_counters.memory_stat_pgmajfault),
                        (const char *[]){ cg_obj->cg_id });
                    cg_obj->cg_counters.memory_stat_pgmajfault = __pgmajfault;

                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_inactive_anon_bytes,
                                   (double)__inactive_anon_bytes,
                                   (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_active_anon_bytes,
                                   (double)__active_anon_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_inactive_file_bytes,
                                   (double)__inactive_file_bytes,
                                   (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_active_file_bytes,
                                   (double)__active_file_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_unevictable_bytes,
                                   (double)__unevictable_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics
                                       .cgroup_metric_memory_stat_hierarchical_memory_limit_bytes,
                                   (double)__hierarchical_memory_limit_bytes,
                                   (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_cache_bytes,
                                   (double)__total_cache_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_rss_bytes,
                                   (double)__total_rss_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_rss_huge_bytes,
                        (double)__total_rss_huge_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_shmem_bytes,
                                   (double)__total_shmem_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_mapped_file_bytes,
                        (double)__total_mapped_file_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_dirty_bytes,
                                   (double)__total_dirty_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_writeback_bytes,
                        (double)__total_writeback_bytes, (const char *[]){ cg_obj->cg_id });

                    prom_counter_add(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_pgpgin,
                        (double)(__total_pgpgin - cg_obj->cg_counters.memory_stat_total_pgpgin),
                        (const char *[]){ cg_obj->cg_id });
                    cg_obj->cg_counters.memory_stat_total_pgpgin = __total_pgpgin;

                    prom_counter_add(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_pgpgout,
                        (double)(__total_pgpgout - cg_obj->cg_counters.memory_stat_total_pgpgout),
                        (const char *[]){ cg_obj->cg_id });
                    cg_obj->cg_counters.memory_stat_total_pgpgout = __total_pgpgout;

                    prom_counter_add(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_pgfault,
                        (double)(__total_pgfault - cg_obj->cg_counters.memory_stat_total_pgfault),
                        (const char *[]){ cg_obj->cg_id });
                    cg_obj->cg_counters.memory_stat_total_pgfault = __total_pgfault;

                    prom_counter_add(cg_obj->cg_metrics.cgroup_metric_memory_stat_total_pgmajfault,
                                     (double)(__total_pgmajfault
                                              - cg_obj->cg_counters.memory_stat_total_pgmajfault),
                                     (const char *[]){ cg_obj->cg_id });
                    cg_obj->cg_counters.memory_stat_total_pgmajfault = __total_pgmajfault;

                    prom_gauge_set(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_inactive_anon_bytes,
                        (double)__total_inactive_anon_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_active_anon_bytes,
                        (double)__total_active_anon_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_inactive_file_bytes,
                        (double)__total_inactive_file_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_active_file_bytes,
                        (double)__total_active_file_bytes, (const char *[]){ cg_obj->cg_id });
                    prom_gauge_set(
                        cg_obj->cg_metrics.cgroup_metric_memory_stat_total_unevictable_bytes,
                        (double)__total_unevictable_bytes, (const char *[]){ cg_obj->cg_id });

                    debug("[PLUGIN_CGROUPS] the cgroup:'%s' memory.stat:'%s' cache:%lu, "
                          "rss:%lu, rss_huge:%lu, mapped_file:%lu, dirty:%lu, writeback:%lu, "
                          "pgpgin:%lu, pgpgout:%lu, pgfault:%lu, pgmajfault:%lu, "
                          "inactive_anon:%lu, active_anon:%lu, inactive_file:%lu, "
                          "active_file:%lu, unevictable:%lu, hierarchical_memory_limit:%lu, "
                          "total_cache:%lu, total_rss:%lu, total_rss_huge:%lu, total_shmem:%lu, "
                          "total_mapped_file:%lu, total_dirty:%lu, total_writeback:%lu, "
                          "total_pgpgin:%lu, total_pgpgout:%lu, total_pgfault:%lu, "
                          "total_pgmajfault:%lu, total_inactive_anon:%lu, total_active_anon:%lu, "
                          "total_inactive_file:%lu, total_active_file:%lu, total_unevictable:%lu",
                          cg_obj->cg_id, cg_obj->memory_stat_filename, __cache_bytes, __rss_bytes,
                          __rss_huge_bytes, __mapped_file_bytes, __dirty_bytes, __writeback_bytes,
                          __pgpgin, __pgpgout, __pgfault, __pgmajfault, __inactive_anon_bytes,
                          __active_anon_bytes, __inactive_file_bytes, __active_file_bytes,
                          __unevictable_bytes, __hierarchical_memory_limit_bytes,
                          __total_cache_bytes, __total_rss_bytes, __total_rss_huge_bytes,
                          __total_shmem_bytes, __total_mapped_file_bytes, __total_dirty_bytes,
                          __total_writeback_bytes, __total_pgpgin, __total_pgpgout, __total_pgfault,
                          __total_pgmajfault, __total_inactive_anon_bytes,
                          __total_active_anon_bytes, __total_inactive_file_bytes,
                          __total_active_file_bytes, __total_unevictable_bytes);
                }
            } else {
                return;
            }
        } else {
            return;
        }
    }

    // read memory.usage_in_bytes
    if (likely(cg_obj->memory_usage_in_bytes_filename)) {
        if (unlikely(
                0
                != read_file_to_uint64(cg_obj->memory_usage_in_bytes_filename, &usage_in_bytes))) {
            // don't continue
            return;
        }
        prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_usage_in_bytes,
                       (double)usage_in_bytes, (const char *[]){ cg_obj->cg_id });
    }

    // read memory.limit_in_bytes
    if (likely(cg_obj->memory_limit_in_bytes_filename)) {
        if (unlikely(
                0
                != read_file_to_uint64(cg_obj->memory_limit_in_bytes_filename, &limit_in_bytes))) {
            // don't continue
            return;
        }
        prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_limit_in_bytes,
                       (double)limit_in_bytes, (const char *[]){ cg_obj->cg_id });
    }

    // read memory.failcnt
    if (likely(cg_obj->memory_failcnt_filename)) {
        if (unlikely(0 != read_file_to_uint64(cg_obj->memory_failcnt_filename, &failcnt))) {
            // don't continue
            return;
        }
        prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_failcnt, (double)failcnt,
                       (const char *[]){ cg_obj->cg_id });
    }

    // read memory.max_usage_in_bytes
    if (likely(cg_obj->memory_max_usage_in_bytes_filename)) {
        if (unlikely(0
                     != read_file_to_uint64(cg_obj->memory_max_usage_in_bytes_filename,
                                            &max_usage_in_bytes))) {
            // don't continue
            return;
        }
        prom_gauge_set(cg_obj->cg_metrics.cgroup_metric_memory_max_usage_in_bytes,
                       (double)max_usage_in_bytes, (const char *[]){ cg_obj->cg_id });
    }

    debug("[PLUGIN_CGROUPS] the cgroup:'%s' memory.usage_in_bytes:%lu, "
          "memory.limit_in_bytes:%lu, memory.failcnt:%lu, memory.max_usage_in_bytes:%lu",
          cg_obj->cg_id, usage_in_bytes, limit_in_bytes, failcnt, max_usage_in_bytes);
}

void collect_cgroup_obj_memory_metrics(struct xm_cgroup_obj *cg_obj) {
    debug("[PLUGIN_CGROUPS] collect cgroup obj:'%s' subsys-memory metrics", cg_obj->cg_id);

    if (cg_type == CGROUPS_V1) {
        __collect_cgroup_v1_memory_metrics(cg_obj);
    }
}