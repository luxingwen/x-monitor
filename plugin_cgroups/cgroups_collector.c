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
 * It clears the `find_flag` of all cgroups
 */
static inline void __unset_all_cgroup_find_flag() {
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
static void __cleanup_all_cgroups() {
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
        }
    }
}

//------------------------------------------------------------------------------

/**
 * It adds a cgroup to the list of cgroups that the plugin is monitoring
 *
 * @param cg_id the cgroup ID
 * @param ctx The context of the plugin.
 *
 * @return A pointer to a struct xm_cgroup_obj
 */
static struct xm_cgroup_obj *__add_cgroup(const char *cg_id, struct plugin_cgroup_ctx *ctx) {
    if (NULL == cg_id || 0 == *cg_id) {
        cg_id = "/";
    }
    debug("[PLUGIN_CGROUPS] create cgroup '%s' add to cgroup_list", cg_id);

    if (__xm_cgroups_mgr.curr_cgroup_count > ctx->max_cgroups_num) {
        error("[PLUGIN_CGROUPS] maximum number of cgroups reached (%d). Not adding cgroup '%s'",
              ctx->max_cgroups_num, cg_id);
        return NULL;
    }

    // 模式匹配，判断是否过滤

    struct xm_cgroup_obj *cg_obj = calloc(1, sizeof(struct xm_cgroup_obj));
    cg_obj->cg_id = sdsnew(cg_id);
    cg_obj->cg_hash = simple_hash(cg_id);

    // 加入链表
    INIT_LIST_HEAD(&cg_obj->l_member);
    list_add_tail(&cg_obj->l_member, &__xm_cgroups_mgr.cg_list);

    return cg_obj;
}

/**
 * It removes the cgroup object from the linked list and frees the memory
 *
 * @param cg_obj The cgroup object to be released.
 *
 * @return A pointer to the cgroup object.
 */
static void __release_cgroup(struct xm_cgroup_obj *cg_obj) {
    if (unlikely(!cg_obj))
        return;

    debug("[PLUGIN_CGROUPS] remove cgroup '%s', find_flag:'%s'", cg_obj->cg_id,
          cg_obj->find_flag ? "true" : "false");

    // 从链表中删除
    list_del(&cg_obj->l_member);

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
static struct xm_cgroup_obj *__find_cgroup(const char *cg_id) {
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

//------------------------------------------------------------------------------

static void __found_cgroup_in_dir(const char *dir, struct plugin_cgroup_ctx *ctx) {
    debug("[PLUGIN_CGROUPS] found cgroup dir '%s'", dir);
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
                // **但是其本身是不过滤的
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
                debug("[PLUGIN_CGROUPS]  filter out cgroup subsystem subdir '%s' (base:'%s', "
                      "relative_path:'%s') according to the cgroups_subpaths_matching",
                      entry->d_name, base_dir, relative_path);
            }
        }
    }

    closedir(dir);
    return ret;
}

//------------------------------------------------------------------------------

void collect_all_cgroups(struct plugin_cgroup_ctx *ctx) {
    debug("[PLUGIN_CGROUPS] collect all cgroups");

    __unset_all_cgroup_find_flag();

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
        }

        if (ctx->cs_blkio_enable) {
            // process cgroup blkio subsystem
        }

        if (ctx->cs_device_enable) {
            // process cgroup device subsystem
        }
    } else {
        // 处理cgroup v2目录结构
    }
}
