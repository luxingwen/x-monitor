/*
 * @Author: CALM.WU
 * @Date: 2022-04-13 15:23:06
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-05-19 17:54:25
 */

#pragma once

#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include "utils/list.h"
#include "utils/consts.h"

#include "app_metrics.h"

struct app_filter_rules;
struct process_status;

// 应用统计结构
struct app_status {
    char             app_name[XM_APP_NAME_SIZE];
    struct list_head l_member;

    pid_t   app_pid;         // 应用主进程ID
    int32_t process_count;   // 应用关联的进程数量

    // 汇总的统计指标
    uint64_t minflt_raw;   // 该任务不需要从硬盘拷数据而发生的缺页（次缺页）的次数
    uint64_t cminflt_raw;   // 累计的该任务的所有的waited-for进程曾经发生的次缺页的次数目
    uint64_t majflt_raw;   // 该任务需要从硬盘拷数据而发生的缺页（主缺页）的次数
    uint64_t cmajflt_raw;   // 累计的该任务的所有的waited-for进程曾经发生的主缺页的次数目
    uint64_t utime_raw;
    uint64_t stime_raw;
    uint64_t cutime_raw;
    uint64_t cstime_raw;
    uint64_t app_cpu_jiffies;
    int32_t  num_threads;
    uint64_t vmsize;   // 当前虚拟内存的实际使用量。
    uint64_t vmrss;   // 应用程序实际占用的物理内存大小，value here is the sum of RssAnon, RssFile,
                      // and RssShmem
    uint64_t rss_anon;    // 匿名RSS内存大小 kB
    uint64_t rss_file;    // 文件RSS内存大小 kB
    uint64_t rss_shmem;   // 共享内存RSS内存大小。
    uint64_t vmswap;      // 应用swap使用量
    uint64_t pss;         // **
    uint64_t pss_anon;
    uint64_t pss_file;
    uint64_t pss_shmem;
    uint64_t uss;   // **
    uint64_t io_logical_bytes_read;
    uint64_t io_logical_bytes_written;
    uint64_t io_read_calls;
    uint64_t io_write_calls;
    uint64_t io_storage_bytes_read;
    uint64_t io_storage_bytes_written;
    int32_t  io_cancelled_write_bytes;
    int32_t  open_fds;

    int16_t max_oom_score;
    int16_t max_oom_score_adj;

    uint64_t nvcsw;
    uint64_t nivcsw;

    prom_collector_t  *app_prom_collector;
    struct app_metrics metrics;
};

// 应用进程关联结构
struct app_assoc_process {
    struct app_status     *as_target;   // 指向应用统计对象
    struct process_status *ps_target;   // 进程统计对象
    uint8_t update;   // 进程是否运行，在一轮采集中如果没有update，表明进程已经退出
};

extern int32_t init_apps_collector();

// 过滤出待收集的应用和其对应的所有进程
extern int32_t update_app_collection(struct app_filter_rules *afr);

// 收集应用的资源使用
extern int32_t collecting_apps_usage();

// 退出清理
extern void free_apps_collector();
