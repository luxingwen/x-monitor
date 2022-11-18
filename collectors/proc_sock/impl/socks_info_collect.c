/*
 * @Author: CALM.WU
 * @Date: 2022-11-15 15:08:58
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-18 16:33:28
 */

// https://guanjunjian.github.io/2017/11/09/study-8-proc-net-tcp-analysis/
// /proc/net/tcp 与 /proc/net/tcp6

#include "socks_info_mgr.h"

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/procfile.h"

static const char *const __proc_tcp_sock = "/proc/net/tcp";
static const char *const __proc_tcp6_sock = "/proc/net/tcp6";
static const char *const __proc_udp_sock = "/proc/net/udp";
static const char *const __proc_udp6_sock = "/proc/net/udp6";
static const char *const __proc_unix_sock = "/proc/net/unix";

static int32_t __collect_tcp_socks_info(struct proc_file *pf, const char *const proc_net_tcp) {
    debug("[PROC_SOCK] start collect tcp:'%s' socks info.", proc_net_tcp);

    pf = procfile_reopen(pf, proc_net_tcp, " \t:", PROCFILE_FLAG_DEFAULT);
    if (unlikely(!pf)) {
        return -1;
    }

    pf = procfile_readall(pf);
    if (unlikely(!pf)) {
        error("[PROC_SOCK] read proc file:'%s' failed.", proc_net_tcp);
        return -1;
    }

    size_t lines = procfile_lines(pf);
    // skip header
    for (size_t l = 1; l < lines; l++) {
        size_t words = procfile_linewords(pf, l);
        if (words < 14) {
            continue;
        }
    }

    return 0;
}

static int32_t __collect_udp_socks_info(struct proc_file *pf, const char *const proc_net_udp) {
    debug("[PROC_SOCK] start collect udp socks info.");
    return 0;
}

static int32_t __collect_unix_socks_info(struct proc_file *pf, const char *const proc_net_unix) {
    debug("[PROC_SOCK] start collect unix socks info.");
    return 0;
}

int32_t do_sock_info_collection() {
    int32_t ret = 0;
    debug("[PROC_SOCK] start collect socks info.");

    ret = __collect_tcp_socks_info(g_proc_sock_info_mgr->proc_net_tcp, __proc_tcp_sock);
    ret += __collect_tcp_socks_info(g_proc_sock_info_mgr->proc_net_tcp6, __proc_tcp6_sock);
    ret += __collect_udp_socks_info(g_proc_sock_info_mgr->proc_net_udp, __proc_udp_sock);
    ret += __collect_udp_socks_info(g_proc_sock_info_mgr->proc_net_udp6, __proc_udp6_sock);
    ret += __collect_unix_socks_info(g_proc_sock_info_mgr->proc_net_unix, __proc_unix_sock);
    return ret == 0 ? 0 : -1;
}