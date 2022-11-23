/*
 * @Author: CALM.WU
 * @Date: 2022-11-18 11:31:54
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-18 11:34:03
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/atomic.h"

#include "collectors/proc_sock/proc_sock.h"

static atomic_t __exit_flag = ATOMIC_INIT(0);

void *collect_routine(void *arg) {
    uint32_t n = *(uint32_t *)arg;

    urcu_memb_register_thread();

    for (size_t i = 0; i < n; i++) {
        debug("---------------------collect %lu----------------------", pthread_self());
        collect_socks_info();
        sleep(3);
    }

    urcu_memb_unregister_thread();

    atomic_inc(&__exit_flag);

    return NULL;
}

void *read_routine(void *arg) {
    urcu_memb_register_thread();

    while (atomic_read(&__exit_flag) == 0) {
        debug("---------------------read %lu----------------------", pthread_self());
        find_batch_sock_info(0, NULL);
        sleep(1);
    }

    debug("---------------------read %lu exit----------------------", pthread_self());
    urcu_memb_unregister_thread();

    return NULL;
}

int32_t main(int32_t argc, char *argv[]) {
    int32_t ret = 0;

    pthread_t tids[3] = { 0, 0, 0 };

    if (log_init("../examples/log.cfg", "proc_sock_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    ret = init_proc_socks();
    if (ret != 0) {
        error("init proc socks failed.");
        return -1;
    }

    pthread_create(&tids[0], NULL, collect_routine, &(uint32_t){ 10 });
    pthread_create(&tids[1], NULL, read_routine, NULL);
    pthread_create(&tids[2], NULL, read_routine, NULL);

    pthread_join(tids[0], NULL);
    pthread_join(tids[1], NULL);
    pthread_join(tids[2], NULL);

    fini_proc_socks();
    log_fini();

    sleep(1);
    return 0;
}