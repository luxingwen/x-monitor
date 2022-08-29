/*
 * @Author: CALM.WU
 * @Date: 2022-08-22 15:03:50
 * @Last Modified by:   CALM.WU
 * @Last Modified time: 2022-08-22 15:03:50
 */

#include "cgroups_collector.h"
#include "routine.h"
#include "utils/clocks.h"
#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"
#include "utils/sds/sds.h"

#include "app_config/app_config.h"

#include "apps_filter_rule.h"
#include "apps_status.h"

static const char *__name = "PLUGIN_CGROUPS";
static const char *__config_name = "collector_plugin_cgroups";
static const char *__plugin_cgroups_config_base_path = "application.collector_plugin_cgroups";

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
    sds css_cpuacct_path;
    sds css_memory_path;
    sds css_cpuset_path;
    sds css_blkio_path;
    sds css_device_path;
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
};

__attribute__((constructor)) static void collector_cgroups_register_routine() {
    fprintf(stderr, "---register_collector_cgroup_routine---\n");
    struct xmonitor_static_routine *xsr =
        (struct xmonitor_static_routine *)calloc(1, sizeof(struct xmonitor_static_routine));
    xsr->name = __name;
    xsr->config_name = __config_name;   //配置文件中节点名
    xsr->enabled = 0;
    xsr->thread_id = &__collector_appstat.thread_id;
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

    debug("[%s] routine init successed", __name);
    return 0;
}

void *cgroup_collector_routine_start(void *arg) {
}

void cgroup_collector_routine_stop() {
}