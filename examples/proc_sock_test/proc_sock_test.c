/*
 * @Author: CALM.WU
 * @Date: 2022-11-18 11:31:54
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-18 11:34:03
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"

#include "collectors/proc_sock/proc_sock.h"

void *collect_routine(void *arg) {
    uint32_t n = *(uint32_t *)arg;

    urcu_memb_register_thread();

    for (size_t i = 0; i < n; i++) {
        debug("---------------------collect %lu----------------------", pthread_self());
        collect_socks_info();
        sleep(3);
    }

    urcu_memb_unregister_thread();

    return NULL;
}

void *read_routine(void *arg) {
    uint32_t n = *(uint32_t *)arg;

    urcu_memb_register_thread();

    for (size_t i = 0; i < n; i++) {
        debug("---------------------read %lu----------------------", pthread_self());
        find_batch_sock_info(0, NULL);
        sleep(1);
    }

    urcu_memb_unregister_thread();

    return NULL;
}

int32_t main(int32_t argc, char *argv[]) {
    int32_t ret = 0;

    pthread_t tids[2] = { 0, 0 };

    if (log_init("../examples/log.cfg", "proc_sock_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    ret = init_proc_socks();
    if (ret != 0) {
        error("init proc socks failed.");
        return -1;
    }

    pthread_create(&tids[0], NULL, collect_routine, &(uint32_t){ 7 });
    pthread_create(&tids[1], NULL, read_routine, &(uint32_t){ 9 });

    pthread_join(tids[0], NULL);
    pthread_join(tids[1], NULL);

    fini_proc_socks();
    log_fini();

    sleep(1);
    return 0;
}