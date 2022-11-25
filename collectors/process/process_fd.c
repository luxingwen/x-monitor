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
#include "sds/sds.h"
#include "collectors/proc_sock/proc_sock.h"

// ls  "/proc/$pid/fd"|wc -l

int32_t collector_process_fd_usage(struct process_status *ps) {
    ps->process_open_fds = 0;

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
    struct dirent   *fd_entry = NULL;

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
                    break;
                case ST_TCP6:
                    break;
                case ST_UDP:
                    break;
                case ST_UDP6:
                    break;
                case ST_UNIX:
                    break;
                }
            }
        }
    }
}

closedir(fds);
fds = NULL;

debug("[PROCESS:fds] process '%d' open fds: %d", ps->pid, ps->process_open_fds);
return 0;
}