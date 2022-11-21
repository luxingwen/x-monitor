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

int32_t main(int32_t argc, char *argv[]) {
    int32_t ret = 0;

    if (log_init("../examples/log.cfg", "proc_sock_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    urcu_memb_register_thread();

    ret = init_proc_socks();
    if (ret != 0) {
        error("init proc socks failed.");
        return -1;
    }

    collect_socks_info();

    fini_proc_socks();
    log_fini();

    urcu_memb_unregister_thread();

    sleep(1);
    return 0;
}