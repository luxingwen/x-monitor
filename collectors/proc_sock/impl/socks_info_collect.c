/*
 * @Author: CALM.WU
 * @Date: 2022-11-15 15:08:58
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-18 18:00:21
 */

// https://guanjunjian.github.io/2017/11/09/study-8-proc-net-tcp-analysis/
// /proc/net/tcp 与 /proc/net/tcp6

#include "socks_info_mgr.h"

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/procfile.h"

static struct sock_file_info {
    enum SOCK_TYPE sock_type;
    const char *   sock_file;
} __sock_file_infos[] = {
    { ST_TCP, "/proc/net/tcp" },   { ST_TCP6, "/proc/net/tcp6" }, { ST_UDP, "/proc/net/udp" },
    { ST_UDP6, "/proc/net/udp6" }, { ST_UNIX, "/proc/net/unix" },
};

static int32_t __collect_tcp_socks_info(struct proc_file *pf, const struct sock_file_info *sfi) {
    debug("[PROC_SOCK] start collect tcp:'%s' socks info.", sfi->sock_file);

    pf = procfile_reopen(pf, sfi->sock_file, " \t:", PROCFILE_FLAG_DEFAULT);
    if (unlikely(!pf)) {
        return -1;
    }

    pf = procfile_readall(pf);
    if (unlikely(!pf)) {
        error("[PROC_SOCK] read proc file:'%s' failed.", sfi->sock_file);
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

static int32_t __collect_udp_socks_info(struct proc_file *pf, const struct sock_file_info *sfi) {
    debug("[PROC_SOCK] start collect udp socks info.");
    return 0;
}

static int32_t __collect_unix_socks_info(struct proc_file *pf, const struct sock_file_info *sfi) {
    debug("[PROC_SOCK] start collect unix socks info.");
    return 0;
}

/**
 * It reads the contents of the /proc/net/tcp, /proc/net/tcp6, /proc/net/udp, /proc/net/udp6, and
 * /proc/net/unix files, and parses the contents of each file to extract information about the
 * sockets
 */
void collect_socks_info_i() {
    debug("[PROC_SOCK] start collect socks info.");

    // 清除所有sock_info查询标志位
    clean_all_sock_info_update_flag();

    // 收集sock信息
    for (size_t i = 0; i < ARRAY_SIZE(__sock_file_infos); i++) {
        struct sock_file_info *sfi = &__sock_file_infos[i];
        if (sfi->sock_type == ST_TCP || sfi->sock_type == ST_TCP6) {
            __collect_tcp_socks_info(NULL, sfi);
        } else if (sfi->sock_type == ST_UDP || sfi->sock_type == ST_UDP6)) {
                __collect_udp_socks_info(NULL, sfi);
            }
        else if (sfi->sock_type == ST_UNIX) {
            __collect_unix_socks_info(NULL, sfi);
        }
    }
    __collect_tcp_socks_info(g_proc_sock_info_mgr->proc_net_tcp, __proc_tcp_sock);
    __collect_tcp_socks_info(g_proc_sock_info_mgr->proc_net_tcp6, __proc_tcp6_sock);
    __collect_udp_socks_info(g_proc_sock_info_mgr->proc_net_udp, __proc_udp_sock);
    __collect_udp_socks_info(g_proc_sock_info_mgr->proc_net_udp6, __proc_udp6_sock);
    __collect_unix_socks_info(g_proc_sock_info_mgr->proc_net_unix, __proc_unix_sock);

    // 释放sock_info查询标志位为0的对象
    delete_all_not_update_sock_info();
}