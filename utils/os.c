/*
 * @Author: CALM.WU
 * @Date: 2022-01-18 11:38:31
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-01-24 17:26:50
 */

#include "common.h"
#include "compiler.h"
#include "os.h"
#include "log.h"
#include "procfile.h"
#include "files.h"
#include "strings.h"
#include "consts.h"

static const char *__def_ipaddr = "0.0.0.0";
static const char *__def_macaddr = "00:00:00:00:00:00";
static const char *__def_hostname = "unknown";

static __thread int32_t __processors = 1;
static __thread char    __hostname[HOST_NAME_MAX] = { 0 };

static const char __no_user[] = "";

const char *get_hostname() {
    if (unlikely(0 == __hostname[0])) {
        if (unlikely(0 != gethostname(__hostname, HOST_NAME_MAX))) {
            strlcpy(__hostname, __def_hostname, HOST_NAME_MAX);
            __hostname[7] = '\0';
        }
    }
    return __hostname;
}

const char *get_ipaddr_by_iface(const char *iface, char *ip_buf, size_t ip_buf_size) {
    if (unlikely(NULL == iface)) {
        return __def_ipaddr;
    }

    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;

    int32_t fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (unlikely(fd < 0)) {
        return __def_ipaddr;
    }

    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    char *ip_addr = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
    strlcpy(ip_buf, ip_addr, ip_buf_size);
    // ip_buf[ip_buf_size - 1] = '\0';

    return ip_buf;
}

const char *get_macaddr_by_iface(const char *iface, char *mac_buf, size_t mac_buf_size) {
    if (unlikely(NULL == iface)) {
        return __def_ipaddr;
    }

    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;

    int32_t fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (unlikely(fd < 0)) {
        return __def_macaddr;
    }

    strlcpy(ifr.ifr_name, iface, IFNAMSIZ);
    ioctl(fd, SIOCGIFHWADDR, &ifr);

    close(fd);

    uint8_t *mac = (uint8_t *)ifr.ifr_hwaddr.sa_data;

    snprintf(mac_buf, mac_buf_size - 1, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);
    mac_buf[mac_buf_size - 1] = '\0';

    return mac_buf;
}

// 锁定内存限制 BCC 已经无条件地将此限制设置为无穷大，但 libbpf 不会自动执行此操作（按照设计）。
int32_t bump_memlock_rlimit(void) {
    struct rlimit rlim_new = {
        .rlim_cur = RLIM_INFINITY,
        .rlim_max = RLIM_INFINITY,
    };

    return setrlimit(RLIMIT_MEMLOCK, &rlim_new);
}

const char *get_username(uid_t uid) {
    struct passwd *pwd = getpwuid(uid);
    if (pwd == NULL) {
        return __no_user;
    }
    return pwd->pw_name;
}

int32_t get_system_cpus() {
    // return sysconf(_SC_NPROCESSORS_ONLN); 加强移植性

    struct proc_file *pf_stat = procfile_open("/proc/stat", NULL, PROCFILE_FLAG_DEFAULT);
    if (unlikely(!pf_stat)) {
        error("Cannot open /proc/stat. Assuming system has %d processors. error: %s", __processors,
              strerror(errno));
        return __processors;
    }

    pf_stat = procfile_readall(pf_stat);
    if (unlikely(!pf_stat)) {
        error("Cannot read /proc/stat. Assuming system has %d __processors.", __processors);
        return __processors;
    }

    __processors = 0;

    for (size_t index = 0; index < procfile_lines(pf_stat); index++) {
        if (!procfile_linewords(pf_stat, index)) {
            continue;
        }

        if (strncmp(procfile_lineword(pf_stat, index, 0), "cpu", 3) == 0) {
            __processors++;
        }
    }

    __processors--;
    if (__processors < 1) {
        __processors = 1;
    }

    procfile_close(pf_stat);

    debug("System has %d __processors.", __processors);

    return __processors;
}

int32_t read_tcp_mem(uint64_t *low, uint64_t *pressure, uint64_t *high) {
    int32_t ret = 0;
    char    rd_buffer[1024] = { 0 };
    char   *start = NULL, *end = NULL;

    ret = read_file("/proc/sys/net/ipv4/tcp_mem", rd_buffer, 1023);
    if (unlikely(ret < 0)) {
        return ret;
    }

    start = rd_buffer;
    // *解析C-stringstr将其内容解释为指定内容的整数base，它以type的值形式返回unsigned long long
    // *int。如果endptr不是空指针，该函数还会设置endptr指向数字后的第一个字符。
    *low = strtoull(start, &end, 10);

    start = end;
    *pressure = strtoull(start, &end, 10);

    start = end;
    *high = strtoull(start, &end, 10);

    debug("TCP MEM low = %lu, pressure = %lu, high = %lu", *low, *pressure, *high);

    return 0;
}

__always_inline int32_t read_tcp_max_orphans(uint64_t *tcp_max_orphans) {
    if (unlikely(read_file_to_uint64("/proc/sys/net/ipv4/tcp_max_orphans", tcp_max_orphans) < 0)) {
        return -1;
    }

    debug("TCP max orphans = %lu", *tcp_max_orphans);
    return 0;
}

static __thread char __proc_pid_cmdline_file[XM_PROC_FILENAME_MAX] = { 0 };

int32_t read_proc_pid_cmdline(pid_t pid, char *cmdline, size_t size) {
    int32_t            fd;
    int32_t            rc = 0;
    static const char *unknown_cmdline = "<unknown>";

    snprintf(__proc_pid_cmdline_file, XM_PROC_FILENAME_MAX - 1, "/proc/%d/cmdline", pid);

    fd = open(__proc_pid_cmdline_file, O_RDONLY | O_NOFOLLOW, 0666);
    if (unlikely(fd == -1)) {
        rc = -1;
        goto exit;
    }

    ssize_t bytes = read(fd, cmdline, size);
    close(fd);

    if (unlikely(bytes < 0)) {
        rc = -2;
        goto exit;
    }

    // ** 要特殊处理
    cmdline[bytes] = '\0';
    for (ssize_t i = 0; i < bytes; i++) {
        if (unlikely(!cmdline[i]))
            cmdline[i] = ' ';
    }

exit:
    if (rc != 0) {
        /*
         * The process went away before we could read its process name. Try
         * to give the user "<unknown>" here, but otherwise they get to look
         * at a blank.
         */
        if (strlcpy(cmdline, unknown_cmdline, size) >= size) {
            rc = -3;
        }
    }

    return rc;
}

uint32_t system_hz = 0;
/**
 * Get the number of ticks per second from the system
 */
void get_system_hz() {
    long ticks;

    if ((ticks = sysconf(_SC_CLK_TCK)) < 0) {
        error("sysconf get _SC_CLK_TCK error: %s", strerror(errno));
        exit(-1);
    }
    system_hz = (uint32_t)ticks;
}

/**
 * It reads the first line of /proc/uptime, and returns the first number in that line, multiplied by
 * 100
 * /proc/uptime的进度是百分之一秒，统一规范为百分之一秒为单位
 * @return The uptime of the system in centiseconds.
 */
uint64_t get_uptime() {
    FILE    *fp = NULL;
    char     line[XM_PROC_LINE_SIZE] = { 0 };
    uint32_t up_sec = 0, up_cent = 0;
    uint64_t uptime = 0;

    if (likely((fp = fopen("/proc/uptime", "r")) != NULL)) {
        if (likely(fgets(line, XM_PROC_LINE_SIZE, fp) != NULL)) {
            if (likely(sscanf(line, "%u.%u", &up_sec, &up_cent) == 2)) {
                uptime = (uint64_t)up_sec * 100 + up_cent;
            }
        }
    }

    if (NULL != fp) {
        fclose(fp);
    }
    return uptime;
}

pid_t system_pid_max = 32768;
/**
 * Read the system's maximum pid value from /proc/sys/kernel/pid_max and return it.
 *
 * @return The maximum number of processes that can be created on the system.
 */
pid_t get_system_pid_max() {
    static uint8_t read = 0;

    if (likely(read)) {
        return system_pid_max;
    }
    read = 1;

    uint64_t max_pid = 0;
    if (unlikely(0 != read_file_to_uint64("/proc/sys/kernel/pid_max", &max_pid))) {
        error("Cannot open file '/proc/sys/kernel/pid_max'. Assuming system supports %d pids",
              system_pid_max);
        return system_pid_max;
    }

    if (unlikely(!max_pid)) {
        error("Cannot parse file '/proc/sys/kernel/pid_max'. Assuming system supports %d pids.",
              system_pid_max);
        return system_pid_max;
    }

    system_pid_max = (pid_t)max_pid;
    return system_pid_max;
}

static const char *const __smaps_tags[] = { "Size:",          "Rss:",           "Pss:",
                                            "Private_Clean:", "Private_Dirty:", "Swap:" };
static const size_t      __smaps_tags_len[] = { 5, 4, 4, 14, 14, 5 };

enum smaps_line_type {
    LINE_IS_NORMAL,
    LINE_IS_VMSIZE,
    LINE_IS_RSS,
    LINE_IS_PSS,
    LINE_IS_USS,
    LINE_IS_SWAP,
};
// https://www.jianshu.com/p/8203457a11cc

int32_t get_process_smaps_info(const char *smaps_path, struct process_smaps_info *info) {
    FILE *f_smaps = fopen(smaps_path, "r");

    if (unlikely(!f_smaps)) {
        error("open smaps '%s' failed", smaps_path);
        memset(info, 0, sizeof(struct process_smaps_info));
        return -1;
    }

    char   *line = NULL, *cursor = NULL, *num = NULL;
    size_t  line_size = 0;
    ssize_t read_size = 0;

    enum smaps_line_type type = LINE_IS_NORMAL;

    while ((read_size = getline(&line, &line_size, f_smaps)) != -1) {
        type = 0;

        if (0 == strncmp(line, __smaps_tags[0], __smaps_tags_len[0])) {
            type = LINE_IS_VMSIZE;
            cursor = line + __smaps_tags_len[0];
        } else if (0 == strncmp(line, __smaps_tags[1], __smaps_tags_len[1])) {
            type = LINE_IS_RSS;
            cursor = line + __smaps_tags_len[1];
        } else if (0 == strncmp(line, __smaps_tags[2], __smaps_tags_len[2])) {
            type = LINE_IS_PSS;
            cursor = line + __smaps_tags_len[2];
        } else if (0 == strncmp(line, __smaps_tags[3], __smaps_tags_len[3])) {
            type = LINE_IS_USS;
            cursor = line + __smaps_tags_len[3];
        } else if (0 == strncmp(line, __smaps_tags[4], __smaps_tags_len[4])) {
            type = LINE_IS_USS;
            cursor = line + __smaps_tags_len[4];
        } else if (0 == strncmp(line, __smaps_tags[5], __smaps_tags_len[5])) {
            type = LINE_IS_SWAP;
            cursor = line + __smaps_tags_len[5];
        } else {
            continue;
        }

        while (*cursor == ' ' || *cursor == '\t')
            cursor++;
        num = cursor;
        while (*cursor >= '0' && *cursor <= '9')
            cursor++;
        if (*cursor != 0) {
            *cursor = 0;
        }

        if (LINE_IS_VMSIZE == type) {
            info->vmsize += str2uint64_t(num);
        } else if (LINE_IS_RSS == type) {
            info->rss += str2uint64_t(num);
        } else if (LINE_IS_PSS == type) {
            info->pss += str2uint64_t(num);
        } else if (LINE_IS_USS == type) {
            info->uss += str2uint64_t(num);
        } else if (LINE_IS_SWAP == type) {
            info->swap += str2uint64_t(num);
        }
    }

    free(line);
    fclose(f_smaps);
    f_smaps = NULL;

    return 0;
}

static void __get_all_childpids(pid_t ppid, struct process_descendant_pids *pd_pids) {
    int32_t  read_size = 0;
    int32_t  child_pids_size = 0;
    char     proc_children_file[XM_PROC_CHILDREN_FILENAME_SIZE] = { 0 };
    char     proc_children_line[XM_PROC_CHILDREN_LINE_SIZE] = { 0 };
    uint64_t children_pids[XM_CHILDPID_COUNT_MAX] = { 0 };

    // ** /proc/pid/task/tid/children读取该文件，分解为pid列表
    snprintf(proc_children_file, XM_PROC_CHILDREN_FILENAME_SIZE, "/proc/%d/task/%d/children", ppid,
             ppid);

    read_size = read_file(proc_children_file, proc_children_line, XM_PROC_CHILDREN_LINE_SIZE - 1);
    if (likely(read_size > 0)) {
        child_pids_size =
            str_split_to_nums(proc_children_line, " ", children_pids, XM_CHILDPID_COUNT_MAX);

        if (likely(child_pids_size > 0)) {
            pd_pids->pids = (pid_t *)realloc(
                pd_pids->pids, sizeof(pid_t) * (pd_pids->pids_size + child_pids_size));
            if (unlikely(!pd_pids->pids)) {
                error("realloc child_pids failed");
                return;
            }
            // child level
            for (int32_t i = 0; i < child_pids_size; i++) {
                pd_pids->pids[pd_pids->pids_size] = (pid_t)children_pids[i];
                pd_pids->pids_size++;
            }

            // grandson level
            for (int32_t i = 0; i < child_pids_size; i++) {
                __get_all_childpids(children_pids[i], pd_pids);
            }
        }
    }
}

int32_t get_process_descendant_pids(pid_t pid, struct process_descendant_pids *pd_pids) {
    if (unlikely(NULL == pd_pids || pid < 0)) {
        return -1;
    }

    __get_all_childpids(pid, pd_pids);

    return pd_pids->pids_size;
}
