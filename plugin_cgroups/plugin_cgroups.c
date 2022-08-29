/*
 * @Author: CALM.WU
 * @Date: 2022-08-22 15:03:50
 * @Last Modified by:   CALM.WU
 * @Last Modified time: 2022-08-22 15:03:50
 */

#include "plugin_cgroups.h"

#include "routine.h"
#include "utils/clocks.h"
#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"
#include "utils/sds/sds.h"
#include "utils/simple_pattern.h"

#include "app_config/app_config.h"

static const char *__name = "PLUGIN_CGROUPS";
static const char *__config_name = "collector_plugin_cgroups";
static const char *__plugin_cgroups_config_base_path = "collector_plugin_cgroups";

struct collector_cgroups {
    int32_t   exit_flag;
    pthread_t thread_id;   // routine执行的线程ids
    int32_t   update_every;
    int32_t   update_every_for_cgroups;
    int32_t   max_cgroups_num;

    // cgroup sub system enable
    bool css_cpuacct_enable;
    bool css_memory_enable;
    bool css_cpuset_enable;
    bool css_blkio_enable;
    bool css_device_enable;

    // cgroup sub system path
    const char *css_cpuacct_path;
    const char *css_memory_path;
    const char *css_cpuset_path;
    const char *css_blkio_path;
    const char *css_device_path;

    // matching pattern
    SIMPLE_PATTERN *cgroups_matching;
    SIMPLE_PATTERN *cgroups_subpaths_matching;
};

static struct collector_cgroups __collector_cgroups = {
    .exit_flag = 0,
    .thread_id = 0,
    .update_every = 1,
    .update_every_for_cgroups = 10,
    .max_cgroups_num = 1000,
    .css_cpuacct_enable = true,
    .css_memory_enable = true,
    .css_cpuset_enable = false,
    .css_blkio_enable = false,
    .css_device_enable = false,
    .cgroups_matching = NULL,
    .cgroups_subpaths_matching = NULL,
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
    // 读取配置，初始化__collector_cgroups
    __collector_cgroups.update_every =
        appconfig_get_member_int(__plugin_cgroups_config_base_path, "update_every", 1);
    __collector_cgroups.update_every_for_cgroups =
        appconfig_get_member_int(__plugin_cgroups_config_base_path, "update_every_for_cgroups", 10);
    __collector_cgroups.max_cgroups_num =
        appconfig_get_member_int(__plugin_cgroups_config_base_path, "max_cgroups_num", 1000);

    __collector_cgroups.css_cpuacct_enable =
        appconfig_get_member_bool(__plugin_cgroups_config_base_path, "css_cpuacct_enable", true);
    __collector_cgroups.css_memory_enable =
        appconfig_get_member_bool(__plugin_cgroups_config_base_path, "css_memory_enable", true);
    __collector_cgroups.css_cpuset_enable =
        appconfig_get_member_bool(__plugin_cgroups_config_base_path, "css_cpuset_enable", false);
    __collector_cgroups.css_blkio_enable =
        appconfig_get_member_bool(__plugin_cgroups_config_base_path, "css_blkio_enable", false);
    __collector_cgroups.css_device_enable =
        appconfig_get_member_bool(__plugin_cgroups_config_base_path, "css_device_enable", false);

    __collector_cgroups.css_cpuacct_path = appconfig_get_member_str(
        __plugin_cgroups_config_base_path, "css_cpuacct_path", "/sys/fs/cgroup/cpuacct");
    __collector_cgroups.css_memory_path = appconfig_get_member_str(
        __plugin_cgroups_config_base_path, "css_memory_path", "/sys/fs/cgroup/memory");
    __collector_cgroups.css_cpuset_path = appconfig_get_member_str(
        __plugin_cgroups_config_base_path, "css_cpuset_path", "/sys/fs/cgroup/cpuset");
    __collector_cgroups.css_blkio_path = appconfig_get_member_str(
        __plugin_cgroups_config_base_path, "css_blkio_path", "/sys/fs/cgroup/blkio");
    __collector_cgroups.css_device_path = appconfig_get_member_str(
        __plugin_cgroups_config_base_path, "css_device_path", "/sys/fs/cgroup/devices");

    debug("[PLUGIN_CGROUPS] cgroup_collector_routine_init: update_every=%d, "
          "update_every_for_cgroups=%d, max_cgroups_num=%d",
          __collector_cgroups.update_every, __collector_cgroups.update_every_for_cgroups,
          __collector_cgroups.max_cgroups_num);

    const char *cgroup_matching_pattern = appconfig_get_member_str(
        __plugin_cgroups_config_base_path, "cgroups_subpaths_matching", NULL);
    if (unlikely(!cgroup_matching_pattern)) {
        error("[PLUGIN_CGROUPS] cgroups_subpaths_matching is not configured");
        return -1;
    } else {
        debug("[PLUGIN_CGROUPS] cgroups_subpaths_matching: %s", cgroup_matching_pattern);
        __collector_cgroups.cgroups_subpaths_matching =
            simple_pattern_create(cgroup_matching_pattern, NULL, SIMPLE_PATTERN_EXACT);
    }

    cgroup_matching_pattern =
        appconfig_get_member_str(__plugin_cgroups_config_base_path, "cgroups_matching", NULL);
    if (unlikely(!cgroup_matching_pattern)) {
        error("[PLUGIN_CGROUPS] cgroups_matching is not configured");
        return -1;
    } else {
        debug("[PLUGIN_CGROUPS] cgroups_matching: %s", cgroup_matching_pattern);
        __collector_cgroups.cgroups_matching =
            simple_pattern_create(cgroup_matching_pattern, NULL, SIMPLE_PATTERN_EXACT);
    }

    debug("[%s] routine init successed", __name);
    return 0;
}

void *cgroup_collector_routine_start(void *arg) {
    debug("[%s] routine, thread id: %lu start", __name, pthread_self());

    usec_t step_usecs = __collector_cgroups.update_every * USEC_PER_SEC;

    struct heartbeat hb;
    heartbeat_init(&hb);

    while (!__collector_cgroups.exit_flag) {
        usec_t now_usecs = now_monotonic_usec();
        //等到下一个update周期
        heartbeat_next(&hb, step_usecs);
    }

    debug("[%s] routine exit", __name);
    return NULL;
}

void cgroup_collector_routine_stop() {
    if (likely(__collector_cgroups.cgroups_matching)) {
        simple_pattern_free(__collector_cgroups.cgroups_matching);
    }

    if (likely(__collector_cgroups.cgroups_subpaths_matching)) {
        simple_pattern_free(__collector_cgroups.cgroups_subpaths_matching);
    }

    debug("[%s] routine has completely stopped", __name);
}