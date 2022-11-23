/*
 * @Author: CALM.WU
 * @Date: 2022-11-15 15:08:58
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-18 18:00:21
 */

// https://guanjunjian.github.io/2017/11/09/study-8-proc-net-tcp-analysis/
// https://gist.github.com/jkstill/5095725
// /proc/net/tcp 与 /proc/net/tcp6

/*
Linux 5.x  /proc/net/tcp
Linux 6.x  /proc/PID/net/tcp

Given a socket:

$ ls -l  /proc/24784/fd/11
lrwx------ 1 jkstill dba 64 Dec  4 16:22 /proc/24784/fd/11 -> socket:[15907701]

Find the address

$ head -1 /proc/24784/net/tcp; grep 15907701 /proc/24784/net/tcp
  sl  local_address rem_address   st  tx_queue  rx_queue tr tm->when  retrnsmt   uid  timeout inode
  46: 010310AC:9C4C 030310AC:1770 01 0100000150:00000000  01:00000019 00000000  1000 0 54165785 4
cd1e6040 25 4 27 3 -1

46: 010310AC:9C4C 030310AC:1770 01
|   |         |   |        |    |--> connection state
|   |         |   |        |------> remote TCP port number
|   |         |   |-------------> remote IPv4 address
|   |         |--------------------> local TCP port number
|   |---------------------------> local IPv4 address
|----------------------------------> number of entry

00000150:00000000 01:00000019 00000000
|        |        |  |        |--> number of unrecovered RTO timeouts
|        |        |  |----------> number of jiffies until timer expires
|        |        |----------------> timer_active (see below)
|        |----------------------> receive-queue
|-------------------------------> transmit-queue

1000 0 54165785 4 cd1e6040 25 4 27 3 -1
|    | |        | |        |  | |  |  |--> slow start size threshold,
|    | |        | |        |  | |  |       or -1 if the treshold
|    | |        | |        |  | |  |       is >= 0xFFFF
|    | |        | |        |  | |  |----> sending congestion window
|    | |        | |        |  | |-------> (ack.quick<<1)|ack.pingpong
|    | |        | |        |  |---------> Predicted tick of soft clock
|    | |        | |        |               (delayed ACK control data)
|    | |        | |        |------------> retransmit timeout
|    | |        | |------------------> location of socket in memory
|    | |        |-----------------------> socket reference count
|    | |-----------------------------> inode
|    |----------------------------------> unanswered 0-window probes
|---------------------------------------------> uid


timer_active:
0 no timer is pending
1 retransmit-timer is pending
2 another timer (e.g. delayed ack or keepalive) is pending
3 this is a socket in TIME_WAIT state. Not all field will contain data.
4 zero window probe timer is pending
*/

#include "sock_info_mgr.h"

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/procfile.h"
#include "utils/strings.h"

static struct sock_file_info {
    enum SOCK_TYPE sock_type;
    const char    *sock_file;
} __sock_file_infos[] = {
    { ST_TCP, "/proc/net/tcp" },   { ST_TCP6, "/proc/net/tcp6" }, { ST_UDP, "/proc/net/udp" },
    { ST_UDP6, "/proc/net/udp6" }, { ST_UNIX, "/proc/net/unix" },
};

static void __sock_print(uint32_t sl, const char *loc_addr, const char *loc_port,
                         const char *rem_addr, const char *rem_port, uint32_t ino, int32_t family) {
    if (family == AF_INET) {
        struct in_addr loc_in, rem_in;

        loc_in.s_addr = strtol(loc_addr, NULL, 16);
        rem_in.s_addr = strtol(rem_addr, NULL, 16);

        debug("[PROC_SOCK]family:AF_INET sl:%d, local: %s:%ld, remote: %s:%ld, ino:%u", sl,
              inet_ntoa(loc_in), strtol(loc_port, NULL, 16), inet_ntoa(rem_in),
              strtol(rem_port, NULL, 16), ino);
    } else if (family == AF_INET6) {
        struct in6_addr loc_in6, rem_in6;
        char            loc_addr6[INET6_ADDRSTRLEN], rem_addr6[INET6_ADDRSTRLEN];

        sscanf(loc_addr, "%08x%08x%08x%08x", &loc_in6.s6_addr32[0], &loc_in6.s6_addr32[1],
               &loc_in6.s6_addr32[2], &loc_in6.s6_addr32[3]);
        sscanf(rem_addr, "%08x%08x%08x%08x", &rem_in6.s6_addr32[0], &rem_in6.s6_addr32[1],
               &rem_in6.s6_addr32[2], &rem_in6.s6_addr32[3]);

        inet_ntop(AF_INET6, &loc_in6, loc_addr6, sizeof(loc_addr6));
        inet_ntop(AF_INET6, &rem_in6, rem_addr6, sizeof(rem_addr6));

        debug("[PROC_SOCK]family:AF_INET6 sl:%d local: %s:%ld, remote: %s:%ld, ino:%u", sl,
              loc_addr6, strtol(loc_port, NULL, 16), rem_addr6, strtol(rem_port, NULL, 16), ino);
    }
}

static int32_t __collect_tcp_socks_info(struct proc_sock_info_mgr   *psim,
                                        const struct sock_file_info *sfi) {
    debug("[PROC_SOCK] start collect tcp:'%s' socks info.", sfi->sock_file);

    int32_t family = (sfi->sock_type == ST_TCP) ? AF_INET : AF_INET6;

    struct proc_file **ppf = &(psim->proc_net_tcp);
    if (sfi->sock_type == ST_TCP6) {
        ppf = &(psim->proc_net_tcp6);
    }

    *ppf = procfile_reopen(*ppf, sfi->sock_file, " \t:", PROCFILE_FLAG_DEFAULT);
    if (unlikely(!ppf)) {
        return -1;
    }

    *ppf = procfile_readall(*ppf);
    if (unlikely(!*ppf)) {
        error("[PROC_SOCK] read proc file:'%s' failed.", sfi->sock_file);
        return -1;
    }

    size_t lines = procfile_lines(*ppf);
    // skip header
    for (size_t l = 1; l < lines; l++) {
        size_t words = procfile_linewords(*ppf, l);
        if (words < 14) {
            continue;
        }

        struct sock_info_node *sin = alloc_sock_info_node();
        // tcp 状态
        uint32_t sl = str2uint32_t(procfile_lineword(*ppf, l, 0));
        sin->si.sock_state = str2uint32_t(procfile_lineword(*ppf, l, 5));
        sin->si.sock_type = sfi->sock_type;
        sin->si.ino = str2uint32_t(procfile_lineword(*ppf, l, 13));
        atomic_set(&sin->is_update, 1);

        const char *loc_addr = procfile_lineword(*ppf, l, 1);
        const char *loc_port = procfile_lineword(*ppf, l, 2);
        const char *rem_addr = procfile_lineword(*ppf, l, 3);
        const char *rem_port = procfile_lineword(*ppf, l, 4);
        //__sock_print(sl, loc_addr, loc_port, rem_addr, rem_port, sin->si.ino, family);

        add_sock_info_node(sin);
    }

    return 0;
}

static int32_t __collect_udp_socks_info(struct proc_sock_info_mgr   *psim,
                                        const struct sock_file_info *sfi) {
    debug("[PROC_SOCK] start collect udp:'%s' socks info.", sfi->sock_file);
    return 0;
}

static int32_t __collect_unix_socks_info(struct proc_sock_info_mgr   *psim,
                                         const struct sock_file_info *sfi) {
    debug("[PROC_SOCK] start collect unix:'%s' socks info.", sfi->sock_file);
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
            __collect_tcp_socks_info(g_proc_sock_info_mgr, sfi);
        } else if (sfi->sock_type == ST_UDP || sfi->sock_type == ST_UDP6) {
            __collect_udp_socks_info(g_proc_sock_info_mgr, sfi);
        } else if (sfi->sock_type == ST_UNIX) {
            __collect_unix_socks_info(g_proc_sock_info_mgr, sfi);
        }
    }

    // 释放sock_info查询标志位为0的对象
    remove_all_not_update_sock_info();
}