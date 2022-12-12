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
    __builtin_memset(&ps->sst, 0, sizeof(ps->sst));

    DIR *fds = opendir(ps->fd_dir_fullname);
    if (unlikely(NULL == fds)) {
        error("[PROCESS:fds] open fds dir '%s' failed.", ps->fd_dir_fullname);
        return -1;
    }

    char lnk_path[32] = { 0 };
    ssize_t lnk_len = 0;
    uint32_t sock_ino = 0;
    size_t dir_size = strlen(ps->fd_dir_fullname);
    struct sock_info si;
    struct dirent *fd_entry = NULL;
    int32_t cwd_fd = dirfd(fds);

    while (NULL != (fd_entry = readdir(fds))) {
        if (likely(isdigit(fd_entry->d_name[0]))) {
            ps->process_open_fds++;

            // fd的全路径
            if (fd_entry->d_type == DT_LNK) {
                // fd的链接路径
                lnk_len = readlinkat(cwd_fd, fd_entry->d_name, lnk_path, 31);
                if (unlikely(-1 == lnk_len)) {
                    continue;
                }
                lnk_path[lnk_len] = '\0';

                // // 判断前缀是否是socket
                if (str_has_pfx(lnk_path, "socket:[")) {
                    // 获取socket的ino
                    if (unlikely(
                            1 != sscanf(lnk_path, "socket:[%u]", &sock_ino))) {
                        continue;
                    }

                    // 获取socket的状态
                    if (likely(0 == find_sock_info(sock_ino, &si))) {
                        switch (si.sock_type) {
                        case ST_TCP:
                            switch (si.sock_state) {
                            case SS_ESTABLISHED:
                                ps->sst.tcp_established++;
                                break;
                            case SS_CLOSE_WAIT:
                                ps->sst.tcp_close_wait++;
                                break;
                            case SS_LISTEN:
                                ps->sst.tcp_listen++;
                                break;
                            }
                            break;
                        case ST_TCP6:
                            switch (si.sock_state) {
                            case SS_ESTABLISHED:
                                ps->sst.tcp6_established++;
                                break;
                            case SS_CLOSE_WAIT:
                                ps->sst.tcp6_close_wait++;
                                break;
                            case SS_LISTEN:
                                ps->sst.tcp6_listen++;
                                break;
                            }
                            break;
                        case ST_UDP:
                            switch (si.sock_state) {
                            case SS_ESTABLISHED:
                                ps->sst.udp_established++;
                                break;
                            case SS_CLOSE:
                                ps->sst.udp_close++;
                                break;
                            }
                            break;
                        case ST_UDP6:
                            switch (si.sock_state) {
                            case SS_ESTABLISHED:
                                ps->sst.udp6_established++;
                                break;
                            case SS_CLOSE:
                                ps->sst.udp6_close++;
                                break;
                            }
                            break;
                        case ST_UNIX:
                            switch (si.sock_state) {
                            case SS_ESTABLISHED:
                                ps->sst.unix_established++;
                                break;
                            case SS_SYN_RECV:
                                ps->sst.unix_recv++;
                                break;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    closedir(fds);
    fds = NULL;

    debug("[PROCESS:fds] process '%d' open fds: %d", ps->pid,
          ps->process_open_fds);
    return 0;
}