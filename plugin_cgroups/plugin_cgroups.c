/*
 * @Author: CALM.WU
 * @Date: 2022-08-22 15:03:50
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-08-31 22:50:47
 */

#include "plugin_cgroups.h"
#include "cgroups_utils.h"
#include "cgroups_def.h"
#include "cgroups_mgr.h"

#include "routine.h"
#include "utils/clocks.h"
#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"
#include "utils/sds/sds.h"
#include "utils/simple_pattern.h"
#include "utils/clocks.h"
#include "utils/os.h"

#include "app_config/app_config.h"

static const char *__name = "PLUGIN_CGROUPS";
static const char *__config_name = "collector_plugin_cgroups";
static const char *__plugin_cgroups_config_base_path = "collector_plugin_cgroups";
static const char *__plugin_cgroups_v1_ss_base_path = "collector_plugin_cgroups.cg_base_path.v1";
static const char *__plugin_cgroups_v2_ss_base_path = "collector_plugin_cgroups.cg_base_path.v2";

struct collector_cgroups {
    sig_atomic_t              exit_flag;
    pthread_t                 thread_id;   // routine执行的线程ids
    int32_t                   update_every;
    int32_t                   update_all_cgroups_interval_secs;
    struct plugin_cgroups_ctx ctx;
};

static struct collector_cgroups __collector_cgroups = {
    .exit_flag = 0,
    .thread_id = 0,
    .update_every = 1,
    .update_all_cgroups_interval_secs = 10,
    {
        .max_cgroups_num = 1000,
        .cs_cpuacct_enable = true,
        .cs_memory_enable = true,
        .cs_cpuset_enable = false,
        .cs_blkio_enable = false,
        .cs_device_enable = false,

        .cs_cpuacct_path = NULL,
        .cs_memory_path = NULL,
        .cs_cpuset_path = NULL,
        .cs_blkio_path = NULL,
        .cs_device_path = NULL,
        .unified_path = NULL,

        .cgroups_matching = NULL,
        .cgroups_subpaths_matching = NULL,
    },
};

__attribute__((constructor)) static void collector_cgroups_register_routine() {
    fprintf(stderr, "---register_collector_cgroup_routine---\n");
    struct xmonitor_static_routine *xsr =
        (struct xmonitor_static_routine *)calloc(1, sizeof(struct xmonitor_static_routine));
    xsr->name = __name;
    xsr->config_name = __config_name;   //配置文件中节点名
    xsr->enabled = 0;
    xsr->thread_id = &__collector_cgroups.thread_id;
    xsr->init_routine = cgroup_collector_routine_init;
    xsr->start_routine = cgroup_collector_routine_start;
    xsr->stop_routine = cgroup_collector_routine_stop;
    register_xmonitor_static_routine(xsr);
}

int32_t cgroup_collector_routine_init() {
    get_cgroups_type();

    if (likely(CGROUPS_FAILED != cg_type)) {
        // 判断系统是否使用了cgroup

        info("[PLUGIN_CGROUPS] CGroup type: [%s]", cg_type == CGROUPS_V1 ? "V1" : "V2");

        // 读取配置，初始化__collector_cgroups
        __collector_cgroups.update_every =
            appconfig_get_member_int(__plugin_cgroups_config_base_path, "update_every", 1);

        __collector_cgroups.update_all_cgroups_interval_secs = appconfig_get_member_int(
            __plugin_cgroups_config_base_path, "update_all_cgroups_interval_secs", 10);

        __collector_cgroups.ctx.max_cgroups_num =
            appconfig_get_member_int(__plugin_cgroups_config_base_path, "max_cgroups_num", 1000);

        debug("[PLUGIN_CGROUPS] cgroup_collector_routine_init: update_every=%d, "
              "update_all_cgroups_interval_secs=%d, max_cgroups_num=%d",
              __collector_cgroups.update_every,
              __collector_cgroups.update_all_cgroups_interval_secs,
              __collector_cgroups.ctx.max_cgroups_num);

        __collector_cgroups.ctx.cs_cpuacct_enable =
            appconfig_get_member_bool(__plugin_cgroups_config_base_path, "cs_cpuacct_enable", true);
        __collector_cgroups.ctx.cs_memory_enable =
            appconfig_get_member_bool(__plugin_cgroups_config_base_path, "cs_memory_enable", true);
        __collector_cgroups.ctx.cs_cpuset_enable =
            appconfig_get_member_bool(__plugin_cgroups_config_base_path, "cs_cpuset_enable", false);
        __collector_cgroups.ctx.cs_blkio_enable =
            appconfig_get_member_bool(__plugin_cgroups_config_base_path, "cs_blkio_enable", false);
        __collector_cgroups.ctx.cs_device_enable =
            appconfig_get_member_bool(__plugin_cgroups_config_base_path, "cs_device_enable", false);

        debug("[PLUGIN_CGROUPS] cs_cpuacct_enable: %d, cs_memory_enable: %d, "
              "cs_cpuset_enable: %d, cs_blkio_enable: %d, cs_device_enable: %d",
              __collector_cgroups.ctx.cs_cpuacct_enable, __collector_cgroups.ctx.cs_memory_enable,
              __collector_cgroups.ctx.cs_cpuset_enable, __collector_cgroups.ctx.cs_blkio_enable,
              __collector_cgroups.ctx.cs_device_enable);

        if (CGROUPS_V1 == cg_type) {
            __collector_cgroups.ctx.cs_cpuacct_path =
                appconfig_get_member_str(__plugin_cgroups_v1_ss_base_path, "cs_cpuacct_path", NULL);
            __collector_cgroups.ctx.cs_memory_path = appconfig_get_member_str(
                __plugin_cgroups_v1_ss_base_path, "cs_memory_path", "/sys/fs/cgroup/memory");
            __collector_cgroups.ctx.cs_cpuset_path = appconfig_get_member_str(
                __plugin_cgroups_v1_ss_base_path, "cs_cpuset_path", "/sys/fs/cgroup/cpuset");
            __collector_cgroups.ctx.cs_blkio_path = appconfig_get_member_str(
                __plugin_cgroups_v1_ss_base_path, "cs_blkio_path", "/sys/fs/cgroup/blkio");
            __collector_cgroups.ctx.cs_device_path = appconfig_get_member_str(
                __plugin_cgroups_v1_ss_base_path, "cs_device_path", "/sys/fs/cgroup/devices");

            debug("[PLUGIN_CGROUPS] cg_v1 cs_cpuacct_path:'%s', cs_memory_path:'%s', "
                  "cs_cpuset_path:'%s', cs_blkio_path:'%s', cs_device_path:'%s'",
                  __collector_cgroups.ctx.cs_cpuacct_path, __collector_cgroups.ctx.cs_memory_path,
                  __collector_cgroups.ctx.cs_cpuset_path, __collector_cgroups.ctx.cs_blkio_path,
                  __collector_cgroups.ctx.cs_device_path);
        } else {
            __collector_cgroups.ctx.unified_path = appconfig_get_member_str(
                __plugin_cgroups_v2_ss_base_path, "unified_path", "/sys/fs/cgroup");
            debug("[PLUGIN_CGROUPS] cg_v2 unified_path:'%s'", __collector_cgroups.ctx.unified_path);
        }

        const char *cgroup_matching_pattern =
            appconfig_get_member_str(__plugin_cgroups_config_base_path, "cgroups_matching", NULL);
        if (unlikely(!cgroup_matching_pattern)) {
            error("[PLUGIN_CGROUPS] cgroups_matching is not configured");
            return -1;
        } else {
            debug("[PLUGIN_CGROUPS] cgroups_matching: %s", cgroup_matching_pattern);
            __collector_cgroups.ctx.cgroups_matching =
                simple_pattern_create(cgroup_matching_pattern, NULL, SIMPLE_PATTERN_EXACT);
        }

        cgroup_matching_pattern = appconfig_get_member_str(__plugin_cgroups_config_base_path,
                                                           "cgroups_subpaths_matching", NULL);
        if (unlikely(!cgroup_matching_pattern)) {
            error("[PLUGIN_CGROUPS] cgroups_subpaths_matching is not configured");
            return -1;
        } else {
            debug("[PLUGIN_CGROUPS] cgroups_subpaths_matching: %s", cgroup_matching_pattern);
            __collector_cgroups.ctx.cgroups_subpaths_matching =
                simple_pattern_create(cgroup_matching_pattern, NULL, SIMPLE_PATTERN_EXACT);
        }
    }

    debug("[%s] routine init successed", __name);
    return 0;
}

void *cgroup_collector_routine_start(void *UNUSED(arg)) {
    debug("[%s] routine, thread id: %lu start", __name, pthread_self());

    usec_t step_usecs = __collector_cgroups.update_every * USEC_PER_SEC;
    usec_t step_usecs_for_update_every_cgroups =
        __collector_cgroups.update_all_cgroups_interval_secs * USEC_PER_SEC;

    usec_t last_update_all_cgroups_usecs = 0;

    if (unlikely(0 != cgroups_mgr_init(&__collector_cgroups.ctx))) {
        return NULL;
    }

    struct heartbeat hb;
    heartbeat_init(&hb);

    while (!__collector_cgroups.exit_flag) {
        usec_t now_usecs = now_monotonic_usec();
        //等到下一个update周期
        heartbeat_next(&hb, step_usecs);

        if (likely(CGROUPS_FAILED != cg_type)) {
            //
            if (!__collector_cgroups.exit_flag
                && (now_usecs - last_update_all_cgroups_usecs
                    >= step_usecs_for_update_every_cgroups)) {
                //收集所有cgroup对象
                cgroups_mgr_start_discovery();
                last_update_all_cgroups_usecs = now_usecs;
            }

            // 采集所有cgroup对象的指标
            cgroups_mgr_collect_metrics();
        }
    }

    cgroups_mgr_fini();

    debug("[%s] routine exit", __name);
    return NULL;
}

void cgroup_collector_routine_stop() {
    debug("[%s] routine ready to stop", __name);

    __collector_cgroups.exit_flag = 1;
    pthread_join(__collector_cgroups.thread_id, NULL);

    if (likely(__collector_cgroups.ctx.cgroups_matching)) {
        simple_pattern_free(__collector_cgroups.ctx.cgroups_matching);
    }

    if (likely(__collector_cgroups.ctx.cgroups_subpaths_matching)) {
        simple_pattern_free(__collector_cgroups.ctx.cgroups_subpaths_matching);
    }

    debug("[%s] routine has completely stopped", __name);
}