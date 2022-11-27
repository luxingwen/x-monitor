/*
 * @Author: CALM.WU
 * @Date: 2022-03-29 14:29:03
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-04-13 16:43:47
 */

#include "process_status.h"

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"
#include "utils/strings.h"
#include "utils/os.h"
#include "collectors/proc_sock/proc_sock.h"

// ls  "/proc/$pid/fd"|wc -l

int32_t collector_process_fd_usage(struct process_status *ps) {
    ps->process_open_fds = 0;
    __builtin_memset(&ps->st, 0, sizeof(ps->st));

    DIR *fds = opendir(ps->fd_dir_fullname);
    if (unlikely(NULL == fds)) {
        error("[PROCESS:fds] open fds dir '%s' failed.", ps->fd_dir_fullname);
        return -1;
    }

    char             fd_path[PATH_MAX] = { 0 };
    char             lnk_path[64] = { 0 };
    ssize_t          lnk_len = 0;
    uint32_t         sock_ino = 0;
    size_t           dir_size = strlen(ps->fd_dir_fullname);
    struct sock_info si;
    struct dirent *  fd_entry = NULL;

    strlcpy(fd_path, ps->fd_dir_fullname, sizeof(fd_path));

    while (NULL != (fd_entry = readdir(fds))) {
        if (likely(isdigit(fd_entry->d_name[0]))) {
            ps->process_open_fds++;

            // fd的全路径
            snprintf(fd_path + dir_size, PATH_MAX - dir_size, "/%s", fd_entry->d_name);
            // fd的链接路径
            lnk_len = readlink(fd_path, lnk_path, 63);
            if (unlikely(-1 == lnk_len)) {
                continue;
            }
            lnk_path[lnk_len] = '\0';

            // // 判断前缀是否是socket
            // if (str_has_pfx(lnk_path, "socket:[")) {
            // 获取socket的ino
            if (unlikely(1 != sscanf(lnk_path, "socket:[%u]", &sock_ino))) {
                continue;
            }

            // 获取socket的状态
            if (likely(0 == find_sock_info(sock_ino, &si))) {
                switch (si.sock_type) {
                case ST_TCP:
                    switch (si.sock_state) {
                    case SS_ESTABLISHED:
                        ps->st.tcp_established++;
                        break;
                    case SS_SYN_SENT:
                        ps->st.tcp_syn_sent++;
                        break;
                    case SS_SYN_RECV:
                        ps->st.tcp_syn_recv++;
                        break;
                    case SS_FIN_WAIT1:
                        ps->st.tcp_fin_wait1++;
                        break;
                    case SS_FIN_WAIT2:
                        ps->st.tcp_fin_wait2++;
                        break;
                    case SS_TIME_WAIT:
                        ps->st.tcp_time_wait++;
                        break;
                    case SS_CLOSE:
                        ps->st.tcp_close++;
                        break;
                    case SS_CLOSE_WAIT:
                        ps->st.tcp_close_wait++;
                        break;
                    case SS_LAST_ACK:
                        ps->st.tcp_last_ack++;
                        break;
                    case SS_LISTEN:
                        ps->st.tcp_listen++;
                        break;
                    case SS_CLOSING:
                        ps->st.tcp_closing++;
                        break;
                    }
                    break;
                case ST_TCP6:
                    switch (si.sock_state) {
                    case SS_ESTABLISHED:
                        ps->st.tcp6_established++;
                        break;
                    case SS_SYN_SENT:
                        ps->st.tcp6_syn_sent++;
                        break;
                    case SS_SYN_RECV:
                        ps->st.tcp6_syn_recv++;
                        break;
                    case SS_FIN_WAIT1:
                        ps->st.tcp6_fin_wait1++;
                        break;
                    case SS_FIN_WAIT2:
                        ps->st.tcp6_fin_wait2++;
                        break;
                    case SS_TIME_WAIT:
                        ps->st.tcp6_time_wait++;
                        break;
                    case SS_CLOSE:
                        ps->st.tcp6_close++;
                        break;
                    case SS_CLOSE_WAIT:
                        ps->st.tcp6_close_wait++;
                        break;
                    case SS_LAST_ACK:
                        ps->st.tcp6_last_ack++;
                        break;
                    case SS_LISTEN:
                        ps->st.tcp6_listen++;
                        break;
                    case SS_CLOSING:
                        ps->st.tcp6_closing++;
                        break;
                    }
                    break;
                case ST_UDP:
                    switch (si.sock_state) {
                    case SS_ESTABLISHED:
                        ps->st.udp_established++;
                        break;
                    case SS_CLOSE:
                        ps->st.udp_close++;
                        break;
                    }
                    break;
                case ST_UDP6:
                    switch (si.sock_state) {
                    case SS_ESTABLISHED:
                        ps->st.udp6_established++;
                        break;
                    case SS_CLOSE:
                        ps->st.udp6_close++;
                        break;
                    }
                    break;
                case ST_UNIX:
                    switch (si.sock_state) {
                    case SS_ESTABLISHED:
                        ps->st.unix_established++;
                        break;
                    case SS_SYN_SENT:
                        ps->st.unix_send++;
                        break;
                    case SS_SYN_RECV:
                        ps->st.unix_recv++;
                        break;
                    }
                    break;
                }
            }
        }
    }
    closedir(fds);
    fds = NULL;

    debug("[PROCESS:fds] process '%d' open fds: %d", ps->pid, ps->process_open_fds);
    return 0;
}