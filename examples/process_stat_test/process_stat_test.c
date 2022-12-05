/*
 * @Author: CALM.WU
 * @Date: 2022-03-31 10:24:17
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-12-04 11:50:01
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/strings.h"
#include "utils/procfile.h"
#include "utils/clocks.h"
#include "utils/mempool.h"
#include "collectors/process/process_status.h"
#include "collectors/proc_sock/proc_sock.h"

// 计算进程的cpu占用 https://www.cnblogs.com/songhaibin/p/13885403.html

static int32_t __sig_exit = 0;
// static const int32_t __def_loop_count = 500000000;
static const char *      __proc_stat_filename = "/proc/stat";
static struct proc_file *__pf_stat = NULL;
// static char              __smaps_file[128];

static void __sig_handler(int sig) {
    __sig_exit = 1;
    debug("SIGINT/SIGTERM received, exiting...");
}

static double __get_total_cpu_jiffies() {
    double user_jiffies,    // 用户态时间
        nice_jiffies,       // nice用户态时间
        system_jiffies,     // 系统态时间
        idle_jiffies,       // 空闲时间, 不包含IO等待时间
        io_wait_jiffies,    // IO等待时间
        irq_jiffies,        // 硬中断时间
        soft_irq_jiffies,   // 软中断时间
        steal_jiffies;   // 虚拟化环境中运行其他操作系统上花费的时间（since Linux 2.6.11）

    /*
        from pidstat
     * Compute the total number of jiffies spent by all processors.
     * NB: Don't add cpu_guest/cpu_guest_nice because cpu_user/cpu_nice
     * already include them.
     */
    user_jiffies = (double)str2uint64_t(procfile_lineword(__pf_stat, 0, 1));
    nice_jiffies = (double)str2uint64_t(procfile_lineword(__pf_stat, 0, 2));
    system_jiffies = (double)str2uint64_t(procfile_lineword(__pf_stat, 0, 3));
    idle_jiffies = (double)str2uint64_t(procfile_lineword(__pf_stat, 0, 4));
    io_wait_jiffies = (double)str2uint64_t(procfile_lineword(__pf_stat, 0, 5));
    irq_jiffies = (double)str2uint64_t(procfile_lineword(__pf_stat, 0, 6));
    soft_irq_jiffies = (double)str2uint64_t(procfile_lineword(__pf_stat, 0, 7));
    steal_jiffies = (double)str2uint64_t(procfile_lineword(__pf_stat, 0, 8));

    return (user_jiffies + nice_jiffies + system_jiffies + idle_jiffies + io_wait_jiffies
            + irq_jiffies + soft_irq_jiffies + steal_jiffies);
}

int32_t main(int32_t argc, char **argv) {
    if (log_init("../examples/log.cfg", "process_stat_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    if (unlikely(argc != 2)) {
        fatal("./process_stat_test <pid>\n");
        return -1;
    }

    urcu_memb_register_thread();

    int32_t ret = init_proc_socks();
    if (ret != 0) {
        error("init proc socks failed.");
        return -1;
    }

    pid_t pid = str2int32_t(argv[1]);
    if (unlikely(0 == pid)) {
        fatal("./process_stat_test <pid>\n");
        return -1;
    }

    if (unlikely(!__pf_stat)) {
        __pf_stat = procfile_open(__proc_stat_filename, " \t:", PROCFILE_FLAG_DEFAULT);
        if (unlikely(!__pf_stat)) {
            error("Cannot open %s", __proc_stat_filename);
            return -1;
        }
        debug("opened '%s'", __proc_stat_filename);
    }

    __pf_stat = procfile_readall(__pf_stat);
    if (unlikely(!__pf_stat)) {
        error("Cannot read %s", __proc_stat_filename);
        return -1;
    }

    struct process_smaps_info psmaps;

    uint32_t cores = get_cpu_cores_num();
    get_system_hz();

    debug("process_stat_test pid: %d", pid);

    struct xm_mempool_s *xmp = xm_mempool_init(sizeof(struct process_status), 1024, 1024);

    struct process_status *ps = new_process_status(pid, xmp);
    if (ps == NULL) {
        error("new_process_status() failed");
        return -1;
    }

    signal(SIGINT, __sig_handler);
    signal(SIGTERM, __sig_handler);

    double curr_total_cpu_jiffies = 0.0, prev_total_cpu_jiffies = 0.0;
    double curr_process_cpu_jiffies = 0.0, prev_process_cpu_jiffies = 0.0;

    while (!__sig_exit) {
        // 采集进程的系统资源

        __pf_stat = procfile_readall(__pf_stat);
        COLLECTOR_PROCESS_USAGE(ps, ret);

        if (unlikely(0 != ret)) {
            debug("process '%d' exit", pid);
            break;
        }

        memset(&psmaps, 0, sizeof(struct process_smaps_info));
        get_mss_from_smaps(pid, &psmaps);

        debug("process: %d, vmszie: %lu kB, rss: %lu kB, pss: %lu kB, pss_anon: %lu kB, pss_file: "
              "%lu kB, pss_shem: %lu kB, uss: %lu kB, swap: %lu kB",
              pid, psmaps.vmsize, psmaps.rss, psmaps.pss, psmaps.pss_anon, psmaps.pss_file,
              psmaps.pss_shmem, psmaps.uss, psmaps.swap);

        prev_process_cpu_jiffies = curr_process_cpu_jiffies;
        curr_process_cpu_jiffies = ps->utime_raw + ps->stime_raw + ps->cutime_raw + ps->cstime_raw;

        prev_total_cpu_jiffies = curr_total_cpu_jiffies;
        curr_total_cpu_jiffies = __get_total_cpu_jiffies();

        // 计算进程占用CPU时间百分比
        double process_cpu_percentage = (curr_process_cpu_jiffies - prev_process_cpu_jiffies)
                                        / (curr_total_cpu_jiffies - prev_total_cpu_jiffies) * 100.0
                                        * (double)cores;
        debug("process: %d, cpu %f%%", pid, process_cpu_percentage);

        sleep_usec(1000000);
        // debug(" ");
    }

    debug("ret: %d", ret);

    free_process_status(ps, xmp);
    procfile_close(__pf_stat);
    xm_mempool_fini(xmp);

    fini_proc_socks();
    urcu_memb_unregister_thread();

    log_fini();

    return 0;
}