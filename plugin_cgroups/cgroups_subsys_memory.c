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

#include "prometheus-client-c/prom.h"

#define __register_new_cgroup_metric(metric, new_func, metric_name, collector, help) \
    do {                                                                             \
        metric = new_func(metric_name, help, 1, (const char *[]){ "cgroup" });       \
        prom_collector_add_metric(collector, metric);                                \
    } while (0)

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

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_failcnt, prom_counter_new,
                                 "cgroup_memory_failcnt", cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_failcnt_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_max_usage_in_bytes,
                                 prom_gauge_new, "cgroup_memory_max_usage_in_bytes",
                                 cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_max_usage_in_bytes_help);

    __register_new_cgroup_metric(cg_obj->cg_metrics.cgroup_metric_memory_swappiness, prom_gauge_new,
                                 "cgroup_memory_swappiness", cg_obj->cg_prom_collector,
                                 __sys_cgroup_memory_swappiness_help);

    debug("[PLUGIN_CGROUPS] init cgroup obj:'%s' subsys-memory metrics success.", cg_obj->cg_id);
}

void init_cgroup_memory_pressure_listener(struct xm_cgroup_obj      *cg_obj,
                                          struct plugin_cgroups_ctx *ctx) {
    char    file_path[PATH_MAX] = { 0 };
    char    line[LINE_MAX] = { 0 };
    int32_t mem_pressure_fd = -1, ctrl_fd = -1;

    if (unlikely(!cg_obj)) {
        return;
    }

    if (cg_type == CGROUPS_V1) {
        cg_obj->cg_memory_pressure_evt_fd = eventfd(0, EFD_NONBLOCK);
        if (unlikely(-1 == cg_obj->cg_memory_pressure_evt_fd)) {
            error("[PLUGIN_CGROUPS] create cgroup obj:'%s' memory eventfd failed, errno:%d, "
                  "error:%s.",
                  cg_obj->cg_id, errno, strerror(errno));
            return;
        }

        snprintf(file_path, sizeof(file_path), "%s/%s/memory.pressure_level", ctx->cs_memory_path,
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

        snprintf(file_path, sizeof(file_path), "%s/%s/cgroup.event_control", ctx->cs_memory_path,
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
        snprintf(line, sizeof(line), "%d %d %s%n", cg_obj->cg_memory_pressure_evt_fd,
                 mem_pressure_fd, ctx->cg_memory_pressure_level, &line_size);
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

    return;

failed_clean:
    if (-1 != cg_obj->cg_memory_pressure_evt_fd) {
        close(cg_obj->cg_memory_pressure_evt_fd);
        cg_obj->cg_memory_pressure_evt_fd = -1;
    }

    if (-1 != mem_pressure_fd) {
        close(mem_pressure_fd);
    }

    if (-1 != ctrl_fd) {
        close(ctrl_fd);
    }
}

void read_cgroup_obj_memory_metrics(struct xm_cgroup_obj *cg_obj) {
    debug("[PLUGIN_CGROUPS] read cgroup obj:'%s' subsys-memory metrics", cg_obj->cg_id);

    if (cg_type == CGROUPS_V1) {
        int32_t  ret = 0;
        uint64_t evtfd_val;

        // 如果counter的值为0，非阻塞模式，会直接返回失败，并把errno的值指纹EINVAL
        ret = read(cg_obj->cg_memory_pressure_evt_fd, &evtfd_val, sizeof(evtfd_val));
        if (-1 == ret) {
            error("[PLUGIN_CGROUPS] read cgroup obj:'%s' memory pressure eventfd failed, "
                  "errno:%d, error:%s.",
                  cg_obj->cg_id, errno, strerror(errno));
            return;
        } else if (0 == ret || ret != sizeof(uint64_t)) {
            error("[PLUGIN_CGROUPS] read cgroup obj:'%s' memory pressure eventfd failed, ret:%d",
                  cg_obj->cg_id, ret);
            return;
        }

        if (ret == sizeof(uint64_t)) {
            debug("[PLUGIN_CGROUPS] read cgroup obj:'%s' memory pressure eventfd success, "
                  "evtfd_val:%lu",
                  cg_obj->cg_id, evtfd_val);
        }
    }
}