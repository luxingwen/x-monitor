/*
 * @Author: CALM.WU
 * @Date: 2022-11-15 16:04:51
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-15 18:35:45
 */

#include "proc_sock_diag.h"
#include "common_def.h"

#include "urcu/urcu-memb.h"

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"

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
    debug("[SOCKS_DIAG] init");

    RUN_ONCE({
        sock_diag_rcu_ht = cds_lfht_new_flavor(8, 8, 0, CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING,
                                               &urcu_memb_flavor, NULL);
        if (unlikely(!sock_diag_rcu_ht)) {
            error("[SOCKS_DIAG] allocating hash table failed");
            return -1;
        }
    });

    return 0;
}

int32_t collect_socks_diag() {
    debug("[SOCKS_DIAG] collect");

    return 0;
}

void fini_socks_diag() {
    debug("[SOCKS_DIAG] fini");
}