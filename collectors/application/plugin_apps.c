/*
 * @Author: CALM.WU
 * @Date: 2022-04-13 15:18:43
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-08-31 22:45:22
 */

#include "plugin_apps.h"

#include "routine.h"
#include "utils/clocks.h"
#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"
#include "urcu/urcu-memb.h"
#include "app_config/app_config.h"
#include "collectors/proc_sock/proc_sock.h"
#include "apps_filter_rule.h"
#include "apps_status.h"

static const char *__name = "PLUGIN_APPSTATUS";
static const char *__config_name = "collector_plugin_appstatus";

struct collector_appstat {
    sig_atomic_t exit_flag;
    pthread_t thread_id; // routine执行的线程ids
    int32_t update_every; // 指标采集时间间隔
    int32_t update_every_for_app; // 应用更新时间间隔
    int32_t update_every_for_filter_rules; // 过滤规则更新时间间隔
    int32_t update_every_for_app_sock_diag; // 应用sock诊断更新时间间隔

    usec_t last_update_for_app_usec;
    usec_t last_update_for_filter_rules_usecs;
    usec_t last_update_for_app_sock_diag_usecs;
};

static struct collector_appstat __collector_appstat = {
    .exit_flag = 0,
    .thread_id = 0,
    .update_every = 1,
    .update_every_for_app = 10,
    .update_every_for_filter_rules = 60,
};

__attribute__((constructor)) static void
collector_appstatus_register_routine() {
    // fprintf(stderr, "---register_collector_appstatus_routine---\n");
    struct xmonitor_static_routine *xsr =
        (struct xmonitor_static_routine *)calloc(
            1, sizeof(struct xmonitor_static_routine));
    xsr->name = __name;
    xsr->config_name = __config_name; //配置文件中节点名
    xsr->enabled = 0;
    xsr->thread_id = &__collector_appstat.thread_id;
    xsr->init_routine = appstat_collector_routine_init;
    xsr->start_routine = appstat_collector_routine_start;
    xsr->stop_routine = appstat_collector_routine_stop;
    register_xmonitor_static_routine(xsr);
}

int32_t appstat_collector_routine_init() {
    // 读取配置文件
    __collector_appstat.update_every =
        appconfig_get_int("collector_plugin_appstatus.update_every", 1);
    __collector_appstat.update_every_for_app = appconfig_get_int(
        "collector_plugin_appstatus.update_every_for_app", 10);
    __collector_appstat.update_every_for_filter_rules = appconfig_get_int(
        "collector_plugin_appstatus.update_every_for_filter_rules", 60);
    __collector_appstat.update_every_for_app_sock_diag = appconfig_get_int(
        "collector_plugin_appstatus.update_every_for_app_sock_diag", 3);

    __collector_appstat.last_update_for_app_usec = 0;
    __collector_appstat.last_update_for_filter_rules_usecs = 0;

    debug("[%s] routine init successed", __name);
    return 0;
}

void *appstat_collector_routine_start(void *UNUSED(arg)) {
    debug("[%s] routine, thread id: %lu start", __name, pthread_self());

    urcu_memb_register_thread();

    init_proc_socks();

    usec_t step_usecs = __collector_appstat.update_every * USEC_PER_SEC;
    usec_t step_usecs_for_app =
        __collector_appstat.update_every_for_app * USEC_PER_SEC;
    usec_t step_usecs_for_filter_rules =
        __collector_appstat.update_every_for_filter_rules * USEC_PER_SEC;
    usec_t step_usecs_for_app_sock_diag =
        __collector_appstat.update_every_for_app_sock_diag * USEC_PER_SEC;

    if (unlikely(0 != init_apps_collector())) {
        urcu_memb_unregister_thread();
        return NULL;
    }

    struct heartbeat hb;
    heartbeat_init(&hb);

    // 构造过滤规则
    struct app_filter_rules *afr = create_filter_rules(__config_name);
    if (unlikely(!afr)) {
        urcu_memb_unregister_thread();
        return NULL;
    }
    __collector_appstat.last_update_for_filter_rules_usecs =
        now_monotonic_usec();

    while (!__collector_appstat.exit_flag) {
        usec_t now_usecs = now_monotonic_usec();
        //等到下一个update周期
        heartbeat_next(&hb, step_usecs);

        // 定时更新过滤规则
        if (!__collector_appstat.exit_flag
            && now_usecs
                       - __collector_appstat.last_update_for_filter_rules_usecs
                   >= step_usecs_for_filter_rules) {
            // 更新过滤规则
            struct app_filter_rules *tmp_afr =
                create_filter_rules(__config_name);
            if (likely(tmp_afr)) {
                free_filter_rules(afr);
                afr = tmp_afr;
            }

            __collector_appstat.last_update_for_filter_rules_usecs = now_usecs;
        }

        // 定时更新应用
        if (!__collector_appstat.exit_flag && afr->rule_count > 0
            && now_usecs - __collector_appstat.last_update_for_app_usec
                   >= step_usecs_for_app) {
            // 清理规则匹配标志，更新应用
            clean_filter_rules(afr);
            // 匹配进程cmdline，找到应用对应进程，更新应用集合
            update_app_collection(afr);
            __collector_appstat.last_update_for_app_usec = now_usecs;
        }

        // 定时采集系统sock诊断信息
        if (!__collector_appstat.exit_flag
            && now_usecs
                       - __collector_appstat.last_update_for_app_sock_diag_usecs
                   >= step_usecs_for_app_sock_diag) {
            collect_socks_info();
            __collector_appstat.last_update_for_app_sock_diag_usecs = now_usecs;
        }

        if (!__collector_appstat.exit_flag && afr->rule_count > 0) {
            collecting_apps_usage();
        }
    }

    free_filter_rules(afr);
    afr = NULL;

    fini_proc_socks();

    urcu_memb_unregister_thread();
    debug("[%s] routine exit", __name);

    return NULL;
}

void appstat_collector_routine_stop() {
    __collector_appstat.exit_flag = 1;
    pthread_join(__collector_appstat.thread_id, NULL);

    free_apps_collector();
    debug("[%s] routine has completely stopped", __name);
    return;
}