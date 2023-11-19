/*
 * @Author: calmwu
 * @Date: 2022-08-31 21:03:38
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-07 15:01:24
 */

#include "cgroups_mgr.h"
#include "cgroups_utils.h"
#include "cgroups_def.h"
#include "cgroups_subsystem.h"

#include "utils/common.h"
#include "utils/log.h"
#include "utils/compiler.h"
#include "utils/strings.h"

#include "prometheus-client-c/prom_collector_t.h"
#include "prometheus-client-c/prom_collector_registry_t.h"
#include "prometheus-client-c/prom_map_i.h"
#include "prometheus-client-c/prom_metric_t.h"
#include "prometheus-client-c/prom_metric_i.h"

static struct xm_cgroup_objs_mgr {
    struct list_head cg_discovery_list; // cgroup 发现列表
    struct list_head cg_collection_list; // cgroup 采集列表
    sig_atomic_t curr_cgroup_count; // 当前采集 cgroup 数量
    int32_t do_discovery; // 条件
    pthread_mutex_t mutex; // 互斥锁
    pthread_cond_t cond; // 条件变量
    pthread_t discovery_thread_id; // 发现线程 id
    sig_atomic_t exit_flag;
    struct plugin_cgroups_ctx *ctx;
} *__xm_cgroup_objs_mgr;

static pthread_mutex_t __init_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * It adds a cgroup to the list of cgroups that the plugin is monitoring
 *
 * @param cg_id the cgroup ID
 * @param ctx The context of the plugin.
 *
 * @return A pointer to a struct xm_cgroup_obj
 */
static struct xm_cgroup_obj *__add_cgroup_obj(const char *cg_id) {
    struct plugin_cgroups_ctx *ctx = __xm_cgroup_objs_mgr->ctx;

    if (NULL == cg_id || 0 == *cg_id) {
        cg_id = "/";
    }

    if (__xm_cgroup_objs_mgr->curr_cgroup_count > ctx->max_cgroups_num) {
        error("[PLUGIN_CGROUPS] maximum number of cgroups reached (%d). Not "
              "adding cgroup '%s'",
              ctx->max_cgroups_num, cg_id);
        return NULL;
    }

    struct xm_cgroup_obj *cg_obj = NULL;

    // 模式匹配，判断是否过滤，cgroups_matching
    int32_t enabled = simple_pattern_matches(ctx->cgroups_matching, cg_id);
    if (likely(enabled)) {
        cg_obj = calloc(1, sizeof(struct xm_cgroup_obj));
        cg_obj->cg_id = sdsnew(cg_id);
        cg_obj->cg_hash = simple_hash(cg_id);
        cg_obj->find_flag = 1;
        cg_obj->mem_pressure_low_level_evt_fd =
            cg_obj->mem_pressure_medium_level_evt_fd =
                cg_obj->mem_pressure_critical_level_evt_fd = -1;
        cg_obj->arl_base_mem_stat = arl_create("cg_mem_stat", NULL, 60);
        // 创建 Prom metrics collector
        cg_obj->cg_prom_collector = prom_collector_new(cg_id);
        // 注册 collector 到默认 registry
        if (unlikely(0
                     != prom_collector_registry_register_collector(
                         PROM_COLLECTOR_REGISTRY_DEFAULT,
                         cg_obj->cg_prom_collector))) {
            error("[PLUGIN_APPSTATUS] register cgroup object '%s' collector to "
                  "default registry "
                  "failed.",
                  cg_id);

            sdsfree(cg_obj->cg_id);
            prom_collector_destroy(cg_obj->cg_prom_collector);
            free(cg_obj);
            return NULL;
        }

        init_cgroup_obj_cpu_metrics(cg_obj);
        init_cgroup_obj_memory_metrics(cg_obj);
        init_cgroup_obj_blkio_metrics(cg_obj);

        INIT_LIST_HEAD(&cg_obj->l_discovery_member);
        INIT_LIST_HEAD(&cg_obj->l_collection_member);
        // 加入发现链表
        list_add_tail(&cg_obj->l_discovery_member,
                      &__xm_cgroup_objs_mgr->cg_discovery_list);

        ++(__xm_cgroup_objs_mgr->curr_cgroup_count);
        info("[PLUGIN_CGROUPS] add new cgroup object '%s' to cgroup discovery "
             "list, current cgroup "
             "object count:%d",
             cg_id, __xm_cgroup_objs_mgr->curr_cgroup_count);
    } else {
        info("[PLUGIN_CGROUPS] filter out cgroup '%s' according to the "
             "configure cgroups_matching",
             cg_id);
    }

    return cg_obj;
}

/**
 * It removes the cgroup object from the linked list and frees the memory
 *
 * @param cg_obj The cgroup object to be released.
 *
 * @return A pointer to the cgroup object.
 */
static void __release_cgroup_obj(struct xm_cgroup_obj *cg_obj) {
    if (unlikely(!cg_obj))
        return;

    --(__xm_cgroup_objs_mgr->curr_cgroup_count);
    debug("[PLUGIN_CGROUPS] remove cgroup '%s', find_flag:'%s', current cgroup "
          "object count:%d",
          cg_obj->cg_id, cg_obj->find_flag ? "true" : "false",
          __xm_cgroup_objs_mgr->curr_cgroup_count);

    if (-1 != cg_obj->mem_pressure_low_level_evt_fd) {
        close(cg_obj->mem_pressure_low_level_evt_fd);
    }

    if (-1 != cg_obj->mem_pressure_medium_level_evt_fd) {
        close(cg_obj->mem_pressure_medium_level_evt_fd);
    }

    if (-1 != cg_obj->mem_pressure_critical_level_evt_fd) {
        close(cg_obj->mem_pressure_critical_level_evt_fd);
    }

    if (likely(cg_obj->arl_base_mem_stat)) {
        arl_free(cg_obj->arl_base_mem_stat);
    }

    // v1
    sdsfree(cg_obj->cpuacct_cpu_stat_filename);
    sdsfree(cg_obj->cpuacct_cpuacct_stat_filename);

    sdsfree(cg_obj->cpuacct_cpu_shares_filename);
    sdsfree(cg_obj->cpuacct_cpu_cfs_period_us_filename);
    sdsfree(cg_obj->cpuacct_cpu_cfs_quota_us_filename);
    sdsfree(cg_obj->cpuacct_usage_all_filename);
    sdsfree(cg_obj->cpuacct_usage_filename);
    sdsfree(cg_obj->cpuacct_usage_user_filename);
    sdsfree(cg_obj->cpuacct_usage_sys_filename);

    sdsfree(cg_obj->memory_stat_filename);
    sdsfree(cg_obj->memory_usage_in_bytes_filename);
    sdsfree(cg_obj->memory_limit_in_bytes_filename);
    sdsfree(cg_obj->memory_failcnt_filename);
    sdsfree(cg_obj->memory_max_usage_in_bytes_filename);

    sdsfree(cg_obj->blkio_throttle_io_service_bytes_filename);
    sdsfree(cg_obj->blkio_throttle_io_serviced_filename);
    sdsfree(cg_obj->blkio_io_merged_filename);
    sdsfree(cg_obj->blkio_io_queued_filename);
    sdsfree(cg_obj->blkio_io_service_bytes_filename);
    sdsfree(cg_obj->blkio_io_serviced_filename);

    // v2
    sdsfree(cg_obj->unified_io_stat_filename);
    sdsfree(cg_obj->unified_cpu_stat_filename);
    sdsfree(cg_obj->unified_memory_stat_filename);
    sdsfree(cg_obj->unified_cpu_max_filename);
    sdsfree(cg_obj->unified_memory_current_filename);
    sdsfree(cg_obj->unified_memory_max_filename);
    sdsfree(cg_obj->unified_cpu_pressure);
    sdsfree(cg_obj->unified_io_pressure);
    sdsfree(cg_obj->unified_memory_pressure);

    free(cg_obj->cg_counters.usage_per_cpus);

    if (likely(cg_obj->cg_prom_collector)) {
        prom_map_delete(PROM_COLLECTOR_REGISTRY_DEFAULT->collectors,
                        cg_obj->cg_id);
    }

    sdsfree(cg_obj->cg_id);
    free(cg_obj);
    cg_obj = NULL;
}

/**
 * It finds a cgroup object in the cgroup manager's list of cgroups
 *
 * @param cg_id The cgroup ID.
 *
 * @return A pointer to the cgroup object.
 */
static struct xm_cgroup_obj *__find_cgroup_obj(const char *cg_id) {
    uint32_t cg_hash = simple_hash(cg_id);

    struct list_head *iter = NULL;
    struct xm_cgroup_obj *cg_obj = NULL;

    __list_for_each(iter, &__xm_cgroup_objs_mgr->cg_discovery_list) {
        cg_obj = list_entry(iter, struct xm_cgroup_obj, l_discovery_member);
        if (cg_obj->cg_hash == cg_hash && 0 == strcmp(cg_obj->cg_id, cg_id)) {
            debug("[PLUGIN_CGROUPS] the cgroup '%s' already exist", cg_id);
            return cg_obj;
        }
    }
    debug("[PLUGIN_CGROUPS] the cgroup '%s' no exist", cg_id);
    return NULL;
}

//------------------------------------------------------------------------------

/**
 * It clears the `find_flag` of all cgroups
 */
static inline void __unset_all_cgroup_obj_find_flag() {
    debug("[PLUGIN_CGROUPS] unset all cgroup find flag");

    struct xm_cgroup_obj *cg_obj = NULL;
    struct list_head *iter = NULL;

    __list_for_each(iter, &__xm_cgroup_objs_mgr->cg_discovery_list) {
        cg_obj = list_entry(iter, struct xm_cgroup_obj, l_discovery_member);
        cg_obj->find_flag = 0;
    }
}

/**
 * It deletes all cgroups that are not found in the current cgroup list
 */
static void __cleanup_all_cgroup_objs() {
    debug("[PLUGIN_CGROUPS] cleanup all unset find flag cgroups");

    struct list_head *iter = NULL, *n = NULL;
    struct xm_cgroup_obj *cg_obj = NULL;

    list_for_each_safe (iter, n, &__xm_cgroup_objs_mgr->cg_discovery_list) {
        cg_obj = list_entry(iter, struct xm_cgroup_obj, l_discovery_member);
        if (0 == cg_obj->find_flag) {
            // 不存在的 cgroup 对象
            debug("[PLUGIN_CGROUPS] cleanup cgroup %s", cg_obj->cg_id);
            // list_del(&cg_obj->l_discovery_member);
            list_del(iter);
            // 释放 xm_cgroup_obj 对象
            __release_cgroup_obj(cg_obj);
        }
    }
}

//------------------------------------------------------------------------------

static void __make_cgroup_obj_metric_files(struct xm_cgroup_obj *cg_obj) {
    struct plugin_cgroups_ctx *ctx = __xm_cgroup_objs_mgr->ctx;

    if (likely(cg_obj)) {
        if (cg_type == CGROUPS_V1) {
            cg_obj->cpuacct_cpu_stat_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpu.stat", ctx->cs_cpuacct_path,
                          cg_obj->cg_id);
            cg_obj->cpuacct_cpuacct_stat_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpuacct.stat",
                          ctx->cs_cpuacct_path, cg_obj->cg_id);

            cg_obj->cpuacct_cpu_shares_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpu.shares", ctx->cs_cpuacct_path,
                          cg_obj->cg_id);
            cg_obj->cpuacct_cpu_cfs_period_us_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpu.cfs_period_us",
                          ctx->cs_cpuacct_path, cg_obj->cg_id);
            cg_obj->cpuacct_cpu_cfs_quota_us_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpu.cfs_quota_us",
                          ctx->cs_cpuacct_path, cg_obj->cg_id);

            cg_obj->cpuacct_usage_all_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpuacct.usage_all",
                          ctx->cs_cpuacct_path, cg_obj->cg_id);

            cg_obj->cpuacct_usage_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpuacct.usage",
                          ctx->cs_cpuacct_path, cg_obj->cg_id);
            cg_obj->cpuacct_usage_user_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpuacct.usage_user",
                          ctx->cs_cpuacct_path, cg_obj->cg_id);
            cg_obj->cpuacct_usage_sys_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpuacct.usage_sys",
                          ctx->cs_cpuacct_path, cg_obj->cg_id);

            cg_obj->memory_stat_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.stat", ctx->cs_memory_path,
                          cg_obj->cg_id);
            cg_obj->memory_usage_in_bytes_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.usage_in_bytes",
                          ctx->cs_memory_path, cg_obj->cg_id);
            cg_obj->memory_limit_in_bytes_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.limit_in_bytes",
                          ctx->cs_memory_path, cg_obj->cg_id);
            cg_obj->memory_failcnt_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.failcnt",
                          ctx->cs_memory_path, cg_obj->cg_id);
            cg_obj->memory_max_usage_in_bytes_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.max_usage_in_bytes",
                          ctx->cs_memory_path, cg_obj->cg_id);

            cg_obj->blkio_throttle_io_service_bytes_filename =
                sdscatfmt(sdsempty(), "%s/%s/blkio.throttle.io_service_bytes",
                          ctx->cs_blkio_path, cg_obj->cg_id);
            cg_obj->blkio_throttle_io_serviced_filename =
                sdscatfmt(sdsempty(), "%s/%s/blkio.throttle.io_serviced",
                          ctx->cs_blkio_path, cg_obj->cg_id);
            cg_obj->blkio_io_merged_filename =
                sdscatfmt(sdsempty(), "%s/%s/blkio.io_merged",
                          ctx->cs_blkio_path, cg_obj->cg_id);
            cg_obj->blkio_io_queued_filename =
                sdscatfmt(sdsempty(), "%s/%s/blkio.io_queued",
                          ctx->cs_blkio_path, cg_obj->cg_id);
        } else if (cg_type == CGROUPS_V2) {
            cg_obj->unified_io_stat_filename = sdscatfmt(
                sdsempty(), "%s/%s/io.stat", ctx->unified_path, cg_obj->cg_id);
            cg_obj->unified_cpu_max_filename = sdscatfmt(
                sdsempty(), "%s/%s/cpu.max", ctx->unified_path, cg_obj->cg_id);
            cg_obj->unified_cpu_stat_filename = sdscatfmt(
                sdsempty(), "%s/%s/cpu.stat", ctx->unified_path, cg_obj->cg_id);
            cg_obj->unified_memory_stat_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.stat", ctx->unified_path,
                          cg_obj->cg_id);
            cg_obj->unified_memory_current_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.current", ctx->unified_path,
                          cg_obj->cg_id);
            cg_obj->unified_memory_max_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.max", ctx->unified_path,
                          cg_obj->cg_id);
            cg_obj->unified_cpu_pressure =
                sdscatfmt(sdsempty(), "%s/%s/cpu.pressure", ctx->unified_path,
                          cg_obj->cg_id);
            cg_obj->unified_io_pressure =
                sdscatfmt(sdsempty(), "%s/%s/io.pressure", ctx->unified_path,
                          cg_obj->cg_id);
            cg_obj->unified_memory_pressure =
                sdscatfmt(sdsempty(), "%s/%s/memory.pressure",
                          ctx->unified_path, cg_obj->cg_id);
        }
    }
}

static void __found_cgroup_in_dir(const char *cg_id) {
    debug("[PLUGIN_CGROUPS] examining cgroup '%s'", cg_id);
    struct plugin_cgroups_ctx *ctx = __xm_cgroup_objs_mgr->ctx;

    struct xm_cgroup_obj *cg_obj = __find_cgroup_obj(cg_id);
    if (!cg_obj) {
        cg_obj = __add_cgroup_obj(cg_id);
        if (cg_obj) {
            // 配置 cgroup 指标文件
            __make_cgroup_obj_metric_files(cg_obj);
            // 配置监控 cgroup memory pressure

            cg_obj->mem_pressure_low_level_evt_fd =
                init_cgroup_memory_pressure_listener(
                    cg_obj, ctx->cs_memory_path, "low");
            cg_obj->mem_pressure_medium_level_evt_fd =
                init_cgroup_memory_pressure_listener(
                    cg_obj, ctx->cs_memory_path, "medium");
            cg_obj->mem_pressure_critical_level_evt_fd =
                init_cgroup_memory_pressure_listener(
                    cg_obj, ctx->cs_memory_path, "critical");
        }
    } else {
        cg_obj->find_flag = 1;
    }
}

static int32_t __find_dir_in_subdirs(const char *base_dir, const char *this,
                                     void (*callback)(const char *)) {
    debug("[PLUGIN_CGROUPS] searching for directories in '%s' (base_dir '%s')",
          this ? this : "/", base_dir);

    if (!this)
        this = base_dir;

    int32_t ret = -1;
    int32_t enabled = 0;
    size_t base_len = strlen(base_dir);

    struct plugin_cgroups_ctx *ctx = __xm_cgroup_objs_mgr->ctx;

    // 子路径
    const char *relative_path = &this[base_len];
    if (0 == *relative_path) {
        relative_path = "/";
    }
    debug("[PLUGIN_CGROUPS] relative path '%s' (base:'%s')", relative_path,
          base_dir);

    // 打开目录
    DIR *dir = opendir(this);
    if (unlikely(!dir)) {
        error("[PLUGIN_CGROUPS] cannot open directory '%s'", this);
        return -1;
    }

    ret = 1;
    // 相对目录就是 cgroup 的名称 例如 cgget /system.slice/run-user-1000.mount
    callback(relative_path);

    struct dirent *entry = NULL;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR
            && ((entry->d_name[0] == '.' && entry->d_name[1] == '\0')
                || (entry->d_name[0] == '.' && entry->d_name[1] == '.'
                    && entry->d_name[2] == '\0'))) {
            // skip . and .. dir
            continue;
        }

        if (entry->d_type == DT_DIR) {
            if (0 == enabled) {
                const char *r = relative_path;
                if (*r == 0)
                    r = "/";

                // **过滤 subsystem 下第一层子目录，根据过滤规则
                // cgroups_subpaths_matching，例如/system.slice 下面的子目录
                // **但是其本身是不过滤的，因为 system.sclice 是系统的
                enabled =
                    simple_pattern_matches(ctx->cgroups_subpaths_matching, r);
            }

            if (enabled) {
                sds cg_path =
                    sdscatfmt(sdsempty(), "%s/%s", this, entry->d_name);
                int32_t ret2 =
                    __find_dir_in_subdirs(base_dir, cg_path, callback);
                if (ret2 > 0) {
                    ret += ret2;
                }
                sdsfree(cg_path);
            } else {
                info("[PLUGIN_CGROUPS] filter out cgroup '%s', first subdir "
                     "'%s' (base:'%s') "
                     "according to the configure cgroups_subpaths_matching",
                     relative_path, entry->d_name, base_dir);
            }
        }
    }

    closedir(dir);
    return ret;
}

//------------------------------------------------------------------------------

void cgroups_discovery() {
    debug("[PLUGIN_CGROUPS] ---------discovery all cgroups---------");

    struct plugin_cgroups_ctx *ctx = __xm_cgroup_objs_mgr->ctx;

    __unset_all_cgroup_obj_find_flag();

    if (CGROUPS_V1 == cg_type) {
        // 处理 cgroup v1 目录结构
        if (ctx->cs_cpuacct_enable) {
            // find cgroup obj in cpucacct subsystem
            if (__find_dir_in_subdirs(ctx->cs_cpuacct_path, NULL,
                                      __found_cgroup_in_dir)
                < 0) {
                error("[PLUGIN_CGROUPS] cannot find cgroup cpucacct subsystem");
            }
        }

        if (ctx->cs_memory_enable) {
            // find cgroup obj in memory subsystem
            if (__find_dir_in_subdirs(ctx->cs_memory_path, NULL,
                                      __found_cgroup_in_dir)
                < 0) {
                error("[PLUGIN_CGROUPS] cannot find cgroup memory subsystem");
            }
        }

        if (ctx->cs_cpuset_enable) {
            // find cgroup obj in cpuset subsystem
            if (__find_dir_in_subdirs(ctx->cs_cpuset_path, NULL,
                                      __found_cgroup_in_dir)
                < 0) {
                error("[PLUGIN_CGROUPS] cannot find cgroup cpuset subsystem");
            }
        }

        if (ctx->cs_blkio_enable) {
            // find cgroup obj in blkio subsystem
            if (__find_dir_in_subdirs(ctx->cs_blkio_path, NULL,
                                      __found_cgroup_in_dir)
                < 0) {
                error("[PLUGIN_CGROUPS] cannot find cgroup blkio subsystem");
            }
        }

        if (ctx->cs_device_enable) {
            // find cgroup obj in device subsystem
            if (__find_dir_in_subdirs(ctx->cs_device_path, NULL,
                                      __found_cgroup_in_dir)
                < 0) {
                error("[PLUGIN_CGROUPS] cannot find cgroup device subsystem");
            }
        }
    } else {
        // 处理 cgroup v2 目录结构
        if (__find_dir_in_subdirs(ctx->unified_path, NULL,
                                  __found_cgroup_in_dir)
            < 0) {
            error("[PLUGIN_CGROUPS] cannot find cgroup device subsystem");
        }
    }
}

//------------------------------------------------------------------------------
static void *__cgroups_discovery_routine(void *UNUSED(arg)) {
    info("[PLUGIN_CGROUPS] cgroups discovery thread started");

    while (!(__xm_cgroup_objs_mgr->exit_flag)) {
        pthread_mutex_lock(&__xm_cgroup_objs_mgr->mutex);
        while (0 == __xm_cgroup_objs_mgr->do_discovery) {
            pthread_cond_wait(&__xm_cgroup_objs_mgr->cond,
                              &__xm_cgroup_objs_mgr->mutex);
        }
        __xm_cgroup_objs_mgr->do_discovery = 0;
        pthread_mutex_unlock(&__xm_cgroup_objs_mgr->mutex);

        if (__xm_cgroup_objs_mgr->exit_flag) {
            break;
        }

        cgroups_discovery();

        pthread_mutex_lock(&__xm_cgroup_objs_mgr->mutex);
        // 清除不存在的 cgroup 对象
        __cleanup_all_cgroup_objs();
        // copy 到采集链表
        INIT_LIST_HEAD(&__xm_cgroup_objs_mgr->cg_collection_list);
        struct list_head *iter = NULL;
        struct xm_cgroup_obj *cg_obj = NULL;
        __list_for_each(iter, &__xm_cgroup_objs_mgr->cg_discovery_list) {
            cg_obj = list_entry(iter, struct xm_cgroup_obj, l_discovery_member);
            INIT_LIST_HEAD(&cg_obj->l_collection_member);
            list_add_tail(&cg_obj->l_collection_member,
                          &__xm_cgroup_objs_mgr->cg_collection_list);
        }
        pthread_mutex_unlock(&__xm_cgroup_objs_mgr->mutex);
    }

    info("[PLUGIN_CGROUPS] cgroups discovery thread exit!");
    return NULL;
}

//------------------------------------------------------------------------------
// 采集 cgroup 各个子系统的指标
static void __read_cgroup_metrics(struct xm_cgroup_obj *cg_obj) {
    debug("[PLUGIN_CGROUPS] read cg_obj:'%s' metrics", cg_obj->cg_id);

    collect_cgroup_obj_cpu_metrics(cg_obj);
    collect_cgroup_obj_memory_metrics(cg_obj);
    collect_cgroup_obj_blkio_metrics(cg_obj);
}

int32_t cgroups_mgr_init(struct plugin_cgroups_ctx *ctx) {
    if (unlikely(NULL == ctx)) {
        error("[PLUGIN_CGROUPS] ctx is NULL");
        return -1;
    }

    int32_t ret = 0;

    if (__xm_cgroup_objs_mgr) {
        return 0;
    } else {
        pthread_mutex_lock(&__init_mutex);

        __xm_cgroup_objs_mgr = calloc(1, sizeof(struct xm_cgroup_objs_mgr));
        if (unlikely(NULL == __xm_cgroup_objs_mgr)) {
            error("[PLUGIN_CGROUPS] calloc struct xm_cgroup_objs_mgr failed!");
            ret = -1;
        } else {
            pthread_mutex_init(&__xm_cgroup_objs_mgr->mutex, NULL);
            pthread_cond_init(&__xm_cgroup_objs_mgr->cond, NULL);
            INIT_LIST_HEAD(&__xm_cgroup_objs_mgr->cg_discovery_list);
            INIT_LIST_HEAD(&__xm_cgroup_objs_mgr->cg_collection_list);
            __xm_cgroup_objs_mgr->curr_cgroup_count = 0;
            __xm_cgroup_objs_mgr->do_discovery = 0;
            __xm_cgroup_objs_mgr->ctx = ctx;
            __xm_cgroup_objs_mgr->exit_flag = 0;
            // 启动 cgroup 发现线程
            if (unlikely(0
                         != pthread_create(
                             &__xm_cgroup_objs_mgr->discovery_thread_id, NULL,
                             __cgroups_discovery_routine, NULL))) {
                error("[PLUGIN_CGROUPS] create cgroup discovery thread "
                      "failed!");
                free(__xm_cgroup_objs_mgr);
                __xm_cgroup_objs_mgr = NULL;
                ret = -1;
            } else {
                debug("[PLUGIN_CGROUPS] cgroup objs manager init successed.");
            }
        }

        pthread_mutex_unlock(&__init_mutex);
    }
    return ret;
}

void cgroups_mgr_start_discovery() {
    pthread_mutex_lock(&__xm_cgroup_objs_mgr->mutex);
    __xm_cgroup_objs_mgr->do_discovery = 1;
    pthread_cond_signal(&__xm_cgroup_objs_mgr->cond);
    debug("[PLUGIN_CGROUPS] trigger start cgroups discovery.");
    pthread_mutex_unlock(&__xm_cgroup_objs_mgr->mutex);
}

void cgroups_mgr_collect_metrics() {
    struct list_head *iter = NULL;
    struct xm_cgroup_obj *cg_obj = NULL;

    pthread_mutex_lock(&__xm_cgroup_objs_mgr->mutex);

    debug("[PLUGIN_CGROUPS] start collect [%d] cgroups metrics.",
          __xm_cgroup_objs_mgr->curr_cgroup_count);

    __list_for_each(iter, &__xm_cgroup_objs_mgr->cg_collection_list) {
        cg_obj = list_entry(iter, struct xm_cgroup_obj, l_collection_member);
        __read_cgroup_metrics(cg_obj);
    }

    pthread_mutex_unlock(&__xm_cgroup_objs_mgr->mutex);
}

void cgroups_mgr_fini() {
    debug("[PLUGIN_CGROUPS] cgroup objs manager ready to stop");

    if (likely(__xm_cgroup_objs_mgr)) {
        // 停止 cgroup 发现线程
        __xm_cgroup_objs_mgr->exit_flag = 1;
        __xm_cgroup_objs_mgr->do_discovery = 1;
        pthread_cond_signal(&__xm_cgroup_objs_mgr->cond);
        pthread_join(__xm_cgroup_objs_mgr->discovery_thread_id, NULL);

        // 释放所有 xm_cgroup_obj
        debug("[PLUGIN_CGROUPS] now free all cgroup object");
        struct list_head *iter = NULL;
        struct list_head *n = NULL;
        struct xm_cgroup_obj *cg_obj = NULL;

        list_for_each_safe (iter, n, &__xm_cgroup_objs_mgr->cg_discovery_list) {
            cg_obj = list_entry(iter, struct xm_cgroup_obj, l_discovery_member);
            list_del(iter);
            __release_cgroup_obj(cg_obj);
            cg_obj = NULL;
        }

        free(__xm_cgroup_objs_mgr);
        __xm_cgroup_objs_mgr = NULL;
    }

    debug("[PLUGIN_CGROUPS] cgroup objs manager fini successed.");
}
