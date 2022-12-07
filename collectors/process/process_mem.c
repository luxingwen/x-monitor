/*
 * @Author: CALM.WU
 * @Date: 2022-03-23 16:19:01
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-04-13 16:43:35
 */

/*
VmRSS                       size of memory portions. It contains the three
                            following parts (VmRSS = RssAnon + RssFile + RssShmem)
RssAnon                     size of resident anonymous memory
RssFile                     size of resident file mappings
RssShmem                    size of resident shmem memory (includes SysV shm,
                            mapping of tmpfs and shared anonymous mappings)

/proc/<pid>/statm  单位是PAGE
 Field    Content
 size     total program size (pages)            (same as VmSize in status)
 resident size of memory portions (pages)       (same as VmRSS in status)
 shared   number of pages that are shared       (i.e. backed by a file)
 trs      number of pages that are 'code'       (not including libs; broken,
                                                        includes data segment)
 lrs      number of pages of library            (always 0 on 2.6)
 drs      number of pages of data/stack         (including libs; broken,
                                                        includes library text)
 dt       number of dirty pages                 (always 0 on 2.6)

https://developer.aliyun.com/article/54405
https://www.jianshu.com/p/8203457a11cc
https://www.cnblogs.com/arnoldlu/p/9375377.html
vss>=rss>=pss>=uss

USS = Private_Clean + Private_Dirty
PSS = USS + (Shared_Clean + Shared_Dirty)/n
是平摊计算后的实际物理使用内存(有些内存会和其他进程共享，例如mmap进来的)。实际上包含下面private_clean+private_dirty，和按比例均分的shared_clean、shared_dirty。
RSS = Private_Clean + Private_Dirty + Shared_Clean + Shared_Dirty

VSS是单个进程全部可访问的虚拟地址空间，其大小可能包括还尚未在内存中驻留的部分。对于确定单个进程实际内存使用大小，VSS用处不大。
RSS是单个进程实际占用的内存大小，RSS不太准确的地方在于它包括该进程所使用共享库全部内存大小。对于一个共享库，可能被多个进程使用，实际该共享库只会被装入内存一次。
进而引出了PSS，PSS相对于RSS计算共享库内存大小是按比例的。N个进程共享，该库对PSS大小的贡献只有1/N。
USS是单个进程私有的内存大小，即该进程独占的内存部分。USS揭示了运行一个特定进程在的真实内存增量大小。如果进程终止，USS就是实际被返还给系统的内存大小。
*/

#include "process_status.h"
//#include "pagemap/pagemap.h"

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/procfile.h"
#include "utils/strings.h"

static const char *const __status_tags[] = { "VmSize:",
                                             "RssAnon:",
                                             "RssFile:",
                                             "RssShmem:",
                                             "VmSwap:",
                                             "voluntary_ctxt_switches:",
                                             "nonvoluntary_ctxt_switches:",
                                             NULL };
static const int32_t     __status_tags_len[] = { 7, 8, 8, 9, 7, 24, 27, 0 };

/**
 * Collects the memory usage of a process
 *
 * @param ps the process_status structure to fill
 *
 * @return Returning 0 means success, non-zero means failure.
 */
int32_t collector_process_mem_usage(struct process_status *ps) {
    char                      mem_status_buf[XM_PROC_CONTENT_BUF_SIZE] = { 0 };
    struct process_smaps_info pid_smaps;

    if (unlikely(NULL == ps || ps->pid <= 0)) {
        error("[PROCESS:mem] process_status is NULL or pid <= 0");
        return -1;
    }

    // pm_process_destroy(pm_proc);
    memset(&pid_smaps, 0, sizeof(struct process_smaps_info));
    get_mss_from_smaps(ps->pid, &pid_smaps);

    // 获取进程的内存使用指标，转换成KB
    ps->vmrss = pid_smaps.rss;
    ps->pss = pid_smaps.pss;
    ps->uss = pid_smaps.uss;
    ps->pss_anon = pid_smaps.pss_anon;
    ps->pss_file = pid_smaps.pss_file;
    ps->pss_shmem = pid_smaps.pss_shmem;

    // 读取/proc/pid/status.RssAnon status.RssFile status.RssShmem
    if (unlikely(0 == ps->mem_status_fd)) {
        ps->mem_status_fd = open(ps->status_full_filename, O_RDONLY);
        if (unlikely(ps->mem_status_fd < 0)) {
            error("[PROCESS:mem] open '%s' failed, errno: %d", ps->status_full_filename, errno);
            return -1;
        }
    } else {
        lseek(ps->mem_status_fd, 0, SEEK_SET);
    }

    ssize_t proc_stauts_buff_len =
        read(ps->mem_status_fd, mem_status_buf, XM_PROC_CONTENT_BUF_SIZE - 1);

    if (unlikely(proc_stauts_buff_len <= 0)) {
        error("[PROCESS:mem] could not read /proc/%d/status, ret: %lu", ps->pid,
              proc_stauts_buff_len);
        close(ps->mem_status_fd);
        ps->mem_status_fd = 0;
        return -1;
    }

    mem_status_buf[proc_stauts_buff_len] = '\0';

    uint64_t *status[] = { &(ps->vmsize), &(ps->rss_anon), &(ps->rss_file), &(ps->rss_shmem),
                           &(ps->vmswap), &(ps->nvcsw),    &(ps->nivcsw) };
    size_t    num_found = 0;

    char *p = mem_status_buf;
    while (*p && num_found < ARRAY_SIZE(status)) {
        int i = 0;
        while (__status_tags[i]) {
            if (strncmp(p, __status_tags[i], __status_tags_len[i]) == 0) {
                p += __status_tags_len[i];
                while (*p == ' ' || *p == '\t')
                    p++;
                char *num = p;
                while (*p >= '0' && *p <= '9')
                    p++;
                if (*p != 0) {
                    *p = 0;
                    p++;
                }
                *(status[i]) = str2uint64_t(num);
                num_found++;
                break;
            }
            i++;
        }
        while (*p && *p != '\n') {
            p++;
        }
        if (*p)
            p++;
    }

    debug("[PROCESS:mem] process: '%d', vmsize: %lu kB, vmrss: %lu kB, vmswap: %lu kB, pss: %lu "
          "kB, pss_anon: %lu kB, pss_file: %lu, pss_shmem: %lu, uss: %lu kB, rss_anon: %lu kB, "
          "rss_file: %lu kB, rss_shmem: %lu kB, voluntary_ctxt_switches: %lu, "
          "involuntary_ctxt_switches: %lu",
          ps->pid, ps->vmsize, ps->vmrss, ps->vmswap, ps->pss, ps->pss_anon, ps->pss_file,
          ps->pss_shmem, ps->uss, ps->rss_anon, ps->rss_file, ps->rss_shmem, ps->nvcsw, ps->nivcsw);

    return 0;
}
