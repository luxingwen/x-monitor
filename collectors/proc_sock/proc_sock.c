/*
 * @Author: CALM.WU
 * @Date: 2022-11-15 16:04:51
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-17 16:18:33
 */

#include "proc_sock.h"

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"

#include "internal/socks_internal.h"

static const char *const __socket_state_name[] = {
    "UNKNOWN",
    [SS_ESTABLISHED] = "ESTABLISHED",
    [SS_SYN_SENT] = "SYN_SENT",
    [SS_SYN_RECV] = "SYN_RECV",
    [SS_FIN_WAIT1] = "FIN_WAIT1",
    [SS_FIN_WAIT2] = "FIN_WAIT2",
    [SS_TIME_WAIT] = "TIME_WAIT",
    [SS_CLOSE] = "CLOSE",
    [SS_CLOSE_WAIT] = "CLOSE_WAIT",
    [SS_LAST_ACK] = "LAST_ACK",
    [SS_LISTEN] = "LISTEN",
    [SS_CLOSING] = "CLOSING", /* Now a valid state */
};

const char *sock_state_name(enum SOCK_STATE ss) {
    if (ss < 1 || ss >= SS_MAX_STATES) {
        return __socket_state_name[0];
    }
    return __socket_state_name[ss];
}

int32_t init_socks_diag() {
    if (unlikely(init_sock_info_mgr() != 0)) {
        return -1;
    }

    debug("[PROC_SOCK] init successed.");
    return 0;
}

int32_t collect_socks_info() {
    debug("[PROC_SOCK] start collect proc socks info");

    return 0;
}

void fini_socks_diag() {
    debug("[PROC_SOCK] fini");
}