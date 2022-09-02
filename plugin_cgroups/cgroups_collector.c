/*
 * @Author: calmwu
 * @Date: 2022-08-31 21:03:38
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-08-31 22:27:37
 */

#include "cgroups_collector.h"
#include "cgroups_utils.h"

#include "utils/common.h"
#include "utils/log.h"
#include "utils/compiler.h"
#include "utils/strings.h"

// cgroup列表
static LIST_HEAD(__xm_cgroup_list);

static struct xm_cgroups_mgr {
    struct list_head cg_list;             // cgroup列表
    int32_t          curr_cgroup_count;   // 当前cgroup数量
} __xm_cgroups_mgr = {
    .cg_list = LIST_HEAD_INIT(__xm_cgroups_mgr.cg_list),
    .curr_cgroup_count = 0,
};

/**
 * It adds a cgroup to the list of cgroups that the plugin is monitoring
 *
 * @param cg_id the cgroup ID
 * @param ctx The context of the plugin.
 *
 * @return A pointer to a struct xm_cgroup_obj
 */
static struct xm_cgroup_obj *__add_cgroup_obj(const char *cg_id, struct plugin_cgroup_ctx *ctx) {
    if (NULL == cg_id || 0 == *cg_id) {
        cg_id = "/";
    }
    debug("[PLUGIN_CGROUPS] create cgroup '%s' add to cgroup_list", cg_id);

    if (__xm_cgroups_mgr.curr_cgroup_count > ctx->max_cgroups_num) {
        error("[PLUGIN_CGROUPS] maximum number of cgroups reached (%d). Not adding cgroup '%s'",
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

        // 加入链表
        INIT_LIST_HEAD(&cg_obj->l_member);
        list_add_tail(&cg_obj->l_member, &__xm_cgroups_mgr.cg_list);

        ++__xm_cgroups_mgr.curr_cgroup_count;
        info("[PLUGIN_CGROUPS] add new cgroup '%s', current cgroup object count:%d", cg_id,
             __xm_cgroups_mgr.curr_cgroup_count);
    } else {
        info("[PLUGIN_CGROUPS] filter out cgroup '%s' according to the configure cgroups_matching");
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

    // 从链表中删除
    list_del(&cg_obj->l_member);
    __xm_cgroups_mgr.curr_cgroup_count--;
    __find_cgroup_obj debug(
        "[PLUGIN_CGROUPS] remove cgroup '%s', find_flag:'%s', current cgroup object count:%d",
        cg_obj->cg_id, cg_obj->find_flag ? "true" : "false", __xm_cgroups_mgr.curr_cgroup_count);

    sdsfree(cg_obj->cg_id);

    sdsfree(cg_obj->cpuacct_cpu_stat_filename);
    sdsfree(cg_obj->cpuacct_cpu_shares_filename);
    sdsfree(cg_obj->cpuacct_cpu_cfs_period_us_filename);
    sdsfree(cg_obj->cpuacct_cpu_cfs_quota_us_filename);
    sdsfree(cg_obj->cpuacct_usage_percpu_filename);
    sdsfree(cg_obj->cpuacct_usage_percpu_user_filename);
    sdsfree(cg_obj->cpuacct_usage_percpu_sys_filename);
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

    sdsfree(cg_obj->unified_io_stat_filename);
    sdsfree(cg_obj->unified_cpu_stat_filename);
    sdsfree(cg_obj->unified_memory_stat_filename);
    sdsfree(cg_obj->unified_memory_current_filename);
    sdsfree(cg_obj->unified_cpu_pressure);
    sdsfree(cg_obj->unified_io_pressure);
    sdsfree(cg_obj->unified_memory_pressure);

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
    debug("[PLUGIN_CGROUPS] find cgroup '%s'", cg_id);

    uint32_t cg_hash = simple_hash(cg_id);

    struct list_head     *iter = NULL;
    struct xm_cgroup_obj *cg_obj = NULL;

    __list_for_each(iter, &__xm_cgroups_mgr.cg_list) {
        cg_obj = list_entry(iter, struct xm_cgroup_obj, l_member);
        if (cg_obj->cg_hash == cg_hash && 0 == strcmp(cg_obj->cg_id, cg_id)) {
            break;
        }
    }
    debug("[PLUGIN_CGROUPS] find cgroup '%s' %s", cg_id, cg_obj ? "success" : "failed");
    return cg_obj;
}

/**
 * Return the number of cgroups currently in the x-monitor.
 *
 * @return The number of cgroups currently in the x-monitor.
 */
int32_t cgroups_count() {
    return __xm_cgroups_mgr.curr_cgroup_count;
}

//------------------------------------------------------------------------------

/**
 * It clears the `find_flag` of all cgroups
 */
static inline void __unset_all_cgroup_obj_find_flag() {
    debug("[PLUGIN_CGROUPS] unset all cgroup find flag");

    struct xm_cgroup_obj *cg_obj = NULL;
    struct list_head     *iter = NULL;

    __list_for_each(iter, &__xm_cgroups_mgr.cg_list) {
        cg_obj = list_entry(iter, struct xm_cgroup_obj, l_member);
        cg_obj->find_flag = false;
    }
}

/**
 * It deletes all cgroups that are not found in the current cgroup list
 */
static void __cleanup_all_cgroup_objs() {
    debug("[PLUGIN_CGROUPS] cleanup all cgroups");

    struct list_head     *iter = NULL;
    struct xm_cgroup_obj *cg_obj = NULL;

    __list_for_each(iter, &__xm_cgroups_mgr.cg_list) {
        cg_obj = list_entry(iter, struct xm_cgroup_obj, l_member);
        if (0 == cg_obj->find_flag) {
            // 不存在的cgroup对象
            debug("[PLUGIN_CGROUPS] cleanup cgroup %s", cg_obj->cg_id);
            list_del(&cg_obj->l_member);
            // 释放xm_cgroup_obj对象
            __release_cgroup_obj(cg_obj);
        }
    }
}

//------------------------------------------------------------------------------

static void __make_cgroup_obj_metric_files(struct xm_cgroup_obj     *cg_obj,
                                           struct plugin_cgroup_ctx *ctx) {
    if (likely(cg_obj)) {
        if (cg_type == CGROUPS_V1) {
            cg_obj->cpuacct_cpu_stat_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpu.stat", ctx->cs_cpuacct_path, cg_obj->cg_id);

            cg_obj->cpuacct_cpu_shares_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpu.shares", ctx->cs_cpuacct_path, cg_obj->cg_id);
            cg_obj->cpuacct_cpu_cfs_period_us_filename = sdscatfmt(
                sdsempty(), "%s/%s/cpu.cfs_period_us", ctx->cs_cpuacct_path, cg_obj->cg_id);
            cg_obj->cpuacct_cpu_cfs_quota_us_filename = sdscatfmt(
                sdsempty(), "%s/%s/cpu.cfs_quota_us", ctx->cs_cpuacct_path, cg_obj->cg_id);

            cg_obj->cpuacct_usage_percpu_filename = sdscatfmt(
                sdsempty(), "%s/%s/cpuacct.usage_percpu", ctx->cs_cpuacct_path, cg_obj->cg_id);
            cg_obj->cpuacct_usage_percpu_user_filename = sdscatfmt(
                sdsempty(), "%s/%s/cpuacct.usage_percpu_user", ctx->cs_cpuacct_path, cg_obj->cg_id);
            cg_obj->cpuacct_usage_percpu_sys_filename = sdscatfmt(
                sdsempty(), "%s/%s/cpuacct.usage_percpu_sys", ctx->cs_cpuacct_path, cg_obj->cg_id);

            cg_obj->cpuacct_usage_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpuacct.usage", ctx->cs_cpuacct_path, cg_obj->cg_id);
            cg_obj->cpuacct_usage_user_filename = sdscatfmt(sdsempty(), "%s/%s/cpuacct.usage_user",
                                                            ctx->cs_cpuacct_path, cg_obj->cg_id);
            cg_obj->cpuacct_usage_sys_filename = sdscatfmt(sdsempty(), "%s/%s/cpuacct.usage_sys",
                                                           ctx->cs_cpuacct_path, cg_obj->cg_id);

            cg_obj->memory_stat_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.stat", ctx->cs_memory_path, cg_obj->cg_id);
            cg_obj->memory_usage_in_bytes_filename = sdscatfmt(
                sdsempty(), "%s/%s/memory.usage_in_bytes", ctx->cs_memory_path, cg_obj->cg_id);
            cg_obj->memory_limit_in_bytes_filename = sdscatfmt(
                sdsempty(), "%s/%s/memory.limit_in_bytes", ctx->cs_memory_path, cg_obj->cg_id);
            cg_obj->memory_failcnt_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.failcnt", ctx->cs_memory_path, cg_obj->cg_id);
            cg_obj->memory_max_usage_in_bytes_filename = sdscatfmt(
                sdsempty(), "%s/%s/memory.max_usage_in_bytes", ctx->cs_memory_path, cg_obj->cg_id);

            cg_obj->blkio_throttle_io_service_bytes_filename =
                sdscatfmt(sdsempty(), "%s/%s/blkio.throttle.io_service_bytes", ctx->cs_blkio_path,
                          cg_obj->cg_id);
            cg_obj->blkio_throttle_io_serviced_filename = sdscatfmt(
                sdsempty(), "%s/%s/blkio.throttle.io_serviced", ctx->cs_blkio_path, cg_obj->cg_id);
            cg_obj->blkio_io_merged_filename =
                sdscatfmt(sdsempty(), "%s/%s/blkio.io_merged", ctx->cs_blkio_path, cg_obj->cg_id);
            cg_obj->blkio_io_queued_filename =
                sdscatfmt(sdsempty(), "%s/%s/blkio.io_queued", ctx->cs_blkio_path, cg_obj->cg_id);
        } else if (cg_type == CGROUPS_V2) {
            cg_obj->unified_io_stat_filename =
                sdscatfmt(sdsempty(), "%s/%s/io.stat", ctx->unified_path, cg_obj->cg_id);
            cg_obj->unified_cpu_stat_filename =
                sdscatfmt(sdsempty(), "%s/%s/cpu.stat", ctx->unified_path, cg_obj->cg_id);
            cg_obj->unified_memory_stat_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.stat", ctx->unified_path, cg_obj->cg_id);
            cg_obj->unified_memory_current_filename =
                sdscatfmt(sdsempty(), "%s/%s/memory.current", ctx->unified_path, cg_obj->cg_id);
            cg_obj->unified_cpu_pressure =
                sdscatfmt(sdsempty(), "%s/%s/cpu.pressure", ctx->unified_path, cg_obj->cg_id);
            cg_obj->unified_io_pressure =
                sdscatfmt(sdsempty(), "%s/%s/io.pressure", ctx->unified_path, cg_obj->cg_id);
            cg_obj->unified_memory_pressure =
                sdscatfmt(sdsempty(), "%s/%s/memory.pressure", ctx->unified_path, cg_obj->cg_id);
        }
    }
}

static void __found_cgroup_in_dir(const char *cg_id, struct plugin_cgroup_ctx *ctx) {
    debug("[PLUGIN_CGROUPS] examining cgroup '%s'", cg_id);

    struct xm_cgroup_obj *cg_obj = __find_cgroup_obj(cg_id);
    if (!cg_obj) {
        cg_obj = __add_cgroup_obj(cg_id, ctx);

        // 配置cgroup指标文件
        __make_cgroup_obj_metric_files(cg_obj);
    } else {
        cg_obj->find_flag = 1;
    }
}

static int32_t __find_dir_in_subdirs(const char *base_dir, const char *this,
                                     void (*callback)(const char *, struct plugin_cgroup_ctx *),
                                     struct plugin_cgroup_ctx *ctx) {

    debug("[PLUGIN_CGROUPS] searching for directories in '%s' (base_dir '%s')", this ? this : "/",
          base_dir);

    if (!this)
        this = base_dir;

    int32_t ret = -1;
    int32_t enabled = 0;
    size_t  base_len = strlen(base_dir);

    // 子路径
    const char *relative_path = &this[base_len];
    if (0 == *relative_path) {
        relative_path = "/";
    }
    debug("[PLUGIN_CGROUPS] relative path '%s'", relative_path);

    // 打开目录
    DIR *dir = opendir(this);
    if (unlikely(!dir)) {
        error("[PLUGIN_CGROUPS] cannot open directory '%s'", this);
        return -1;
    }

    ret = 1;
    // 相对目录就是cgroup的名称 例如cgget /system.slice/run-user-1000.mount
    callback(relative_path, ctx);

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

                // **过滤subsystem下第一层子目录，根据过滤规则cgroups_subpaths_matching，例如/system.slice下面的子目录
                // **但是其本身是不过滤的,因为system.sclice是系统的
                enabled = simple_pattern_matches(ctx->cgroups_subpaths_matching, r);
            }

            if (enabled) {
                sds     cg_path = sdscatfmt(sdsempty(), "%s/%s", this, entry->d_name);
                int32_t ret2 = __find_dir_in_subdirs(base_dir, cg_path, callback, ctx);
                if (ret2 > 0) {
                    ret += ret2;
                }
                sdsfree(cg_path);
            } else {
                debug("[PLUGIN_CGROUPS] filter out cgroup '%s', first subdir '%s' (base:'%s') "
                      "according to the configure cgroups_subpaths_matching",
                      relative_path, entry->d_name, base_dir);
            }
        }
    }

    closedir(dir);
    return ret;
}

//------------------------------------------------------------------------------

void collect_all_cgroups(struct plugin_cgroup_ctx *ctx) {
    debug("[PLUGIN_CGROUPS] collect all cgroups");

    __unset_all_cgroup_obj_find_flag();

    if (CGROUPS_V1 == cg_type) {
        // 处理cgroup v1目录结构
        if (ctx->cs_cpuacct_enable) {
            // process cgroup cpucacct subsystem
            if (__find_dir_in_subdirs(ctx->cs_cpuacct_path, NULL, __found_cgroup_in_dir, ctx) < 0) {
                error("[PLUGIN_CGROUPS] cannot find cgroup cpucacct subsystem");
            }
        }

        if (ctx->cs_memory_enable) {
            // process cgroup memory subsystem
            if (__find_dir_in_subdirs(ctx->cs_memory_path, NULL, __found_cgroup_in_dir, ctx) < 0) {
                error("[PLUGIN_CGROUPS] cannot find cgroup memory subsystem");
            }
        }

        if (ctx->cs_cpuset_enable) {
            // process cgroup cpuset subsystem
            if (__find_dir_in_subdirs(ctx->cs_cpuset_path, NULL, __found_cgroup_in_dir, ctx) < 0) {
                error("[PLUGIN_CGROUPS] cannot find cgroup cpuset subsystem");
            }
        }

        if (ctx->cs_blkio_enable) {
            // process cgroup blkio subsystem
            if (__find_dir_in_subdirs(ctx->cs_blkio_path, NULL, __found_cgroup_in_dir, ctx) < 0) {
                error("[PLUGIN_CGROUPS] cannot find cgroup blkio subsystem");
            }
        }

        if (ctx->cs_device_enable) {
            // process cgroup device subsystem
            if (__find_dir_in_subdirs(ctx->cs_device_path, NULL, __found_cgroup_in_dir, ctx) < 0) {
                error("[PLUGIN_CGROUPS] cannot find cgroup device subsystem");
            }
        }
    } else {
        // 处理cgroup v2目录结构
        if (__find_dir_in_subdirs(ctx->unified_path, NULL, __found_cgroup_in_dir, ctx) < 0) {
            error("[PLUGIN_CGROUPS] cannot find cgroup device subsystem");
        }
    }
}
