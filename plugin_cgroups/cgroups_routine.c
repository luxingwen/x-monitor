/*
 * @Author: calmwu
 * @Date: 2022-08-31 21:03:38
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-08-31 21:27:51
 */

#include "cgroups_routine.h"

#include "utils/common.h"
#include "utils/log.h"
#include "utils/compiler.h"
#include "utils/threads.h"

static struct cgoups_update_routine {
    pthread_mutext_t mutex;
    pthread_cond_t   cond;
    pthread_t        routine_id;
    int32_t          start_update_all;
    sig_atomic_t     exit_flag;
    sig_atomic_t     init_flag;
} __cgoups_update_routine = {
    .routine_id = 0,
    .start_update_all = 0,
    .exit_flag = 0,
    .init_flag = 0,
};

void *update_all_cgroups_routine(void *args) {
    return NULL;
}

int32_t start_cgroups_update_routine(void *args) {
    xm_mutex_init(&__cgoups_update_routine.mutex);

    if (!__cgoups_update_routine.init_flag) {
        xm_mutex_lock(&__cgoups_update_routine.mutex);

        // 启动
        __cgoups_update_routine.init_flag = 1;

        xm_mutex_unlock(&__cgoups_update_routine.mutex);
        return 0;
    }
    error("cgroups_update_routine is already running!");
    return -1;
}

void stop_cgroups_update_routine() {
    __cgroups_update_routine.exit_flag = 1;
    pthread_join(__cgroups_update_routine.routine_id, NULL);

    debug("[PLUGIN_CGROUPS] cgroups_update_routine has completely stopped");
}

void notify_cgroups_update_now() {
    // 触发条件变量
}
