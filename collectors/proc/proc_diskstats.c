/*
 * @Author: CALM.WU
 * @Date: 2021-11-30 14:59:07
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-04-24 15:15:41
 */

// https://www.kernel.org/doc/Documentation/iostats.txt
// https://www.kernel.org/doc/Documentation/ABI/testing/procfs-diskstats
// https://www.twblogs.net/a/5eef2321fcc715ab3d918c05

#include "plugin_proc.h"

#include "prometheus-client-c/prom.h"

#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/procfile.h"
#include "utils/strings.h"

#include "app_config/app_config.h"

static const char       *__def_proc_diskstat_filename = "/proc/diskstats";
static const char       *__cfg_proc_diskstat_filename = NULL;
static struct proc_file *__pf_diskstats = NULL;

struct io_stats {
    /*4  (rd_ios)读完成次数 ----- 读磁盘的次数，成功完成读的总次数*/
    uint64_t rd_ios;
    /*5
     * 合并读操作的次数。如果两个读操作读取相邻的数据块时，可以被合并成一个，以提高效率。合并的操作通常是I/O
     * scheduler（也叫elevator）负责的 */
    uint64_t rd_merges;
    /*6 读取的扇区次数 */
    uint64_t rd_sectors;
    /*7
     读操作消耗的时间（以毫秒为单位）。每个读操作从__make_request()开始计时，到end_that_request_last()为止，包括了在队列中等待的时间
     of milliseconds spent reading */
    uint64_t rd_ticks;
    /*8 写完成次数 */
    uint64_t wr_ios;
    /*9 合并写完成次数 */
    uint64_t wr_merges;
    /*10 写扇区次数 */
    uint64_t wr_sectors;
    /*11 寫操作花費的毫秒數  ---
     * 寫花費的毫秒數，這是所有寫操作所花費的毫秒數（用__make_request()到end_that_request_last()測量）
     */
    uint64_t wr_ticks;
    /*12 of I/Os currently in progress
     * 正在處理的輸入/輸出請求數 --
     * -I/O的當前進度，只有這個域應該是0。當請求被交給適當的request_queue_t時增加和請求完成時減小。*/
    uint64_t ios_pgr;
    /*13
     * 请注意io_ticks与rd_ticks(字段#4)和wr_ticks(字段#8)的区别，rd_ticks和wr_ticks是把每一个I/O所消耗的时间累加在一起，因为硬盘设备通常可以并行处理多个I/O，所以rd_ticks和wr_ticks往往会比自然时间大。
     * 执行I/O所花费的毫秒数，在实际计算时，字段#9(ios_pgr)不为零的时候io_ticks保持计时，字段#9(in_flight)为零的时候io_ticks停止计时*/
    uint64_t ios_total_ticks;
    /*14 输入/输出操作花费的加权毫秒数 -----
     * 加权，花在I/O操作上的毫秒数，在每次I/O开始，I/O结束，I/O合并时这个域都会增加。这可以给I/O完成时间和存储那些可以累积的提供一个便利的测量标准。*/
    uint64_t weighted_io_ticks;
    /*15 of discard operations issued to the device */
    uint64_t dc_ios;
    /*16 of discard requests merged */
    uint64_t dc_merges;
    /*17 of sectors discarded */
    uint64_t dc_sectors;
    /*18 Time of discard requests in queue */
    uint64_t dc_ticks;
    /*19 flush requests completed successfully */
    uint64_t fl_ios;
    /*20 time spent flushing */
    uint64_t fl_ticks;
};

struct io_device {
    char device_name[XM_DEV_NAME_MAX];   //

    uint32_t       major;
    uint32_t       minor;
    uint32_t       device_hash;
    enum disk_type dev_tp;
    int32_t        sector_size;

    struct io_stats stats[2];
    uint32_t        curr_stats_id;
    uint32_t        prev_stats_id;

    struct io_device *next;
};

//
#if GCC_VERSION >= 50100
static const int32_t __kernel_sector_size = 512;
#else
#define __kernel_sector_size 512;
#endif
// static const double __kernel_sector_kb = (double)__kernel_sector_size / 1024.0;
//
static struct io_device *__iodev_list = NULL;

static prom_counter_t *__metric_node_disk_reads_completed_total = NULL,
                      *__metric_node_disk_reads_merged_total = NULL,
                      *__metric_node_disk_read_bytes_total = NULL,
                      *__metric_node_disk_read_time_seconds_total = NULL,
                      *__metric_node_disk_writes_completed_total = NULL,
                      *__metric_node_disk_writes_merged_total = NULL,
                      *__metric_node_disk_written_bytes_total = NULL,
                      *__metric_node_disk_write_time_seconds_total = NULL,
                      *__metric_node_disk_io_time_seconds_total = NULL,
                      *__metric_node_disk_io_time_weighted_seconds_total = NULL,
                      *__metric_node_disk_discards_completed_total = NULL,
                      *__metric_node_disk_discards_merged_total = NULL,
                      *__metric_node_discarded_sectors_total = NULL,
                      *__metric_node_disk_discard_time_seconds_total = NULL,
                      *UNUSED(__metric_node_disk_end) = NULL;

static prom_gauge_t *__metric_node_disk_io_now = NULL, *__metric_node_disk_iostat_tps = NULL,
                    *__metric_node_disk_iostat_read_requests_second = NULL,
                    *__metric_node_disk_iostat_write_requests_second = NULL,
                    *__metric_node_disk_iostat_read_kilobytes_second = NULL,
                    *__metric_node_disk_iostat_write_kilobytes_second = NULL,
                    *__metric_node_disk_iostat_read_requests_merged_second = NULL,
                    *__metric_node_disk_iostat_write_requests_merged_second = NULL,
                    *__metric_node_disk_iostat_r_await_msecs = NULL,
                    *__metric_node_disk_iostat_w_await_msecs = NULL,
                    *__metric_node_disk_iostat_await_msecs = NULL,
                    *__metric_node_disk_iostat_areq_sz = NULL,
                    *__metric_node_disk_iostat_rareq_sz = NULL,
                    *__metric_node_disk_iostat_wareq_sz = NULL,
                    *__metric_node_disk_iostat_aqu_sz = NULL,
                    *__metric_node_disk_iostat_util = NULL;

static enum disk_type __get_device_type(const char *dev_name, uint32_t major, uint32_t minor) {
    enum disk_type dt = DISK_TYPE_UNKNOWN;

    char buffer[XM_DEV_NAME_MAX + 1];
    snprintf(buffer, XM_DEV_NAME_MAX, "/sys/block/%s", dev_name);
    if (likely(access(buffer, R_OK) == 0)) {
        // assign it here, but it will be overwritten if it is not a physical disk
        dt = DISK_TYPE_PHYSICAL;
    } else {
        snprintf(buffer, XM_DEV_NAME_MAX, "/sys/dev/block/%u:%u/partition", major, minor);
        if (likely(access(buffer, R_OK) == 0)) {
            dt = DISK_TYPE_PARTITION;
        } else {
            snprintf(buffer, FILENAME_MAX, "/sys/dev/block/%u:%u/slaves", major, minor);
            DIR *dirp = opendir(buffer);
            if (likely(dirp != NULL)) {
                struct dirent *dp;
                while ((dp = readdir(dirp))) {
                    // . and .. are also files in empty folders.
                    if (unlikely(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)) {
                        continue;
                    }

                    dt = DISK_TYPE_VIRTUAL;
                    // Stop the loop after we found one file.
                    break;
                }
                if (unlikely(closedir(dirp) == -1))
                    error("Unable to close dir %s", buffer);
            }
        }
    }

    return dt;
}

static struct io_device *__get_device(char *device_name, uint32_t major, uint32_t minor) {
    struct io_device *dev = NULL;

    // 计算hash值
    uint32_t hash = bkrd_hash(device_name, strlen(device_name));

    // 查找磁盘设备，看是否已经存在
    for (dev = __iodev_list; dev != NULL; dev = dev->next) {
        if (dev->major == major && dev->minor == minor && dev->device_hash == hash) {
            return dev;
        }
    }

    dev = (struct io_device *)calloc(1, sizeof(struct io_device));
    if (unlikely(!dev)) {
        exit(-1);
    }

    strlcpy(dev->device_name, device_name, XM_DEV_NAME_MAX);
    dev->major = major;
    dev->minor = minor;
    dev->device_hash = hash;
    dev->dev_tp = __get_device_type(device_name, major, minor);
    dev->sector_size = get_block_device_sector_size(device_name);
    if (unlikely(dev->sector_size < 0)) {
        dev->sector_size = __kernel_sector_size;
    }
    dev->curr_stats_id = dev->prev_stats_id = 0;

    debug("[PLUGIN_PROC:proc_diskstats] %s:%u:%u:%u", device_name, major, minor, dev->sector_size);

    if (unlikely(!__iodev_list)) {
        __iodev_list = dev;
    } else {
        // 添加在末尾
        struct io_device *last;
        for (last = __iodev_list; last->next != NULL; last = last->next)
            ;
        last->next = dev;
    }

    return dev;
}

int32_t init_collector_proc_diskstats() {
    __metric_node_disk_reads_completed_total =
        prom_collector_registry_must_register_metric(prom_counter_new(
            "node_disk_reads_completed_total", "The total number of reads completed successfully.",
            1, (const char *[]){ "device" }));

    __metric_node_disk_reads_merged_total = prom_collector_registry_must_register_metric(
        prom_counter_new("node_disk_reads_merged_total", "The total number of reads merged.", 1,
                         (const char *[]){ "device" }));

    __metric_node_disk_read_bytes_total =
        prom_collector_registry_must_register_metric(prom_counter_new(
            "node_disk_read_bytes_total", "The total number of bytes read successfully.", 1,
            (const char *[]){ "device" }));

    __metric_node_disk_read_time_seconds_total =
        prom_collector_registry_must_register_metric(prom_counter_new(
            "node_disk_read_time_seconds_total", "The total number of seconds spent by all reads.",
            1, (const char *[]){ "device" }));

    __metric_node_disk_writes_completed_total =
        prom_collector_registry_must_register_metric(prom_counter_new(
            "node_disk_writes_completed_total",
            "The total number of writes completed successfully.", 1, (const char *[]){ "device" }));

    __metric_node_disk_writes_merged_total = prom_collector_registry_must_register_metric(
        prom_counter_new("node_disk_writes_merged_total", "The number of writes merged.", 1,
                         (const char *[]){ "device" }));

    __metric_node_disk_written_bytes_total =
        prom_collector_registry_must_register_metric(prom_counter_new(
            "node_disk_written_bytes_total", "The total number of bytes written successfully.", 1,
            (const char *[]){ "device" }));

    __metric_node_disk_write_time_seconds_total = prom_collector_registry_must_register_metric(
        prom_counter_new("node_disk_write_time_seconds_total",
                         "This is the total number of seconds spent by all writes.", 1,
                         (const char *[]){ "device" }));

    __metric_node_disk_io_now = prom_collector_registry_must_register_metric(
        prom_gauge_new("node_disk_io_now", "The number of I/Os currently in progress.", 1,
                       (const char *[]){ "device" }));

    __metric_node_disk_io_time_seconds_total = prom_collector_registry_must_register_metric(
        prom_counter_new("node_disk_io_time_seconds_total", "Total seconds spent doing I/Os.", 1,
                         (const char *[]){ "device" }));

    __metric_node_disk_io_time_weighted_seconds_total =
        prom_collector_registry_must_register_metric(prom_counter_new(
            "node_disk_io_time_weighted_seconds_total",
            "The weighted # of seconds spent doing I/Os.", 1, (const char *[]){ "device" }));

    __metric_node_disk_discards_completed_total = prom_collector_registry_must_register_metric(
        prom_counter_new("node_disk_discards_completed_total",
                         "The total number of discards completed successfully.", 1,
                         (const char *[]){ "device" }));

    __metric_node_disk_discards_merged_total = prom_collector_registry_must_register_metric(
        prom_counter_new("node_disk_discards_merged_total", "The total number of discards merged.",
                         1, (const char *[]){ "device" }));

    __metric_node_discarded_sectors_total =
        prom_collector_registry_must_register_metric(prom_counter_new(
            "node_discarded_sectors_total", "The total number of sectors discarded successfully.",
            1, (const char *[]){ "device" }));

    __metric_node_disk_discard_time_seconds_total = prom_collector_registry_must_register_metric(
        prom_counter_new("node_disk_discard_time_seconds_total",
                         "This is the total number of seconds spent by all discards.", 1,
                         (const char *[]){ "device" }));

    __metric_node_disk_iostat_tps = prom_collector_registry_must_register_metric(prom_gauge_new(
        "node_disk_iostat_tps",
        "Indicate the number of transfers per second that were issued to the device.", 1,
        (const char *[]){ "device" }));

    __metric_node_disk_iostat_read_requests_second =
        prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_disk_iostat_read_requests_second",
            "The number (after merges) of read requests completed per second for the device.", 1,
            (const char *[]){ "device" }));

    __metric_node_disk_iostat_write_requests_second =
        prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_disk_iostat_write_requests_second",
            "The number (after merges) of write requests completed per second for the device.", 1,
            (const char *[]){ "device" }));

    __metric_node_disk_iostat_read_kilobytes_second = prom_collector_registry_must_register_metric(
        prom_gauge_new("node_disk_iostat_read_kbytes_second",
                       "The number of kilobytes read from the device per second.", 1,
                       (const char *[]){ "device" }));

    __metric_node_disk_iostat_write_kilobytes_second = prom_collector_registry_must_register_metric(
        prom_gauge_new("node_disk_iostat_write_kbytes_second",
                       "The number of kilobytes written to the device per second.", 1,
                       (const char *[]){ "device" }));

    __metric_node_disk_iostat_read_requests_merged_second =
        prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_disk_iostat_read_request_merged_second",
            "The number of read requests merged per second that were queued to the device.", 1,
            (const char *[]){ "device" }));

    __metric_node_disk_iostat_write_requests_merged_second =
        prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_disk_iostat_write_request_merged_second",
            "The number of write requests merged per second that were queued to the device.", 1,
            (const char *[]){ "device" }));

    __metric_node_disk_iostat_r_await_msecs = prom_collector_registry_must_register_metric(
        prom_gauge_new("node_disk_iostat_r_await",
                       "The average time (in milliseconds) for read requests issued to the device "
                       "to be served. This includes the time spent by the requests in queue and "
                       "the time spent servicing them.",
                       1, (const char *[]){ "device" }));

    __metric_node_disk_iostat_w_await_msecs = prom_collector_registry_must_register_metric(
        prom_gauge_new("node_disk_iostat_w_await",
                       "The average time (in milliseconds) for write requests issued to the device "
                       "to be served. This includes the time spent by the requests in queue and "
                       "the time spent servicing them.",
                       1, (const char *[]){ "device" }));

    __metric_node_disk_iostat_await_msecs = prom_collector_registry_must_register_metric(
        prom_gauge_new("node_disk_iostat_await",
                       "The average time (in milliseconds) for I/O requests issued to the device "
                       "to be served. This includes the time spent by the requests in queue and "
                       "the time spent servicing them.",
                       1, (const char *[]){ "device" }));

    __metric_node_disk_iostat_aqu_sz = prom_collector_registry_must_register_metric(
        prom_gauge_new("node_disk_iostat_aqu_sz",
                       "The average queue length of the requests that were issued to the device.",
                       1, (const char *[]){ "device" }));

    __metric_node_disk_iostat_areq_sz = prom_collector_registry_must_register_metric(prom_gauge_new(
        "node_disk_iostat_avg_request_kbytes",
        "The average size (in kilobytes) of the I/O requests that were issued to the device.", 1,
        (const char *[]){ "device" }));

    __metric_node_disk_iostat_rareq_sz =
        prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_disk_iostat_avg_request_read_kbytes",
            "The average size (in kilobytes) of the read requests that were issued to the device.",
            1, (const char *[]){ "device" }));

    __metric_node_disk_iostat_wareq_sz =
        prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_disk_iostat_avg_request_write_kbytes",
            "The average size (in kilobytes) of the write requests that were issued to the device.",
            1, (const char *[]){ "device" }));

    __metric_node_disk_iostat_util = prom_collector_registry_must_register_metric(
        prom_gauge_new("node_disk_iostat_util",
                       "Percentage of elapsed time during which I/O requests were issued to the "
                       "device (bandwidth utilization for the device).",
                       1, (const char *[]){ "device" }));

    debug("[PLUGIN_PROC:proc_diskstats] init successed");
    return 0;
}

int32_t collector_proc_diskstats(int32_t UNUSED(update_every), usec_t dt, const char *config_path) {
    debug("[PLUGIN_PROC:proc_diskstats] config:%s running", config_path);

    if (unlikely(!__cfg_proc_diskstat_filename)) {
        __cfg_proc_diskstat_filename =
            appconfig_get_member_str(config_path, "monitor_file", __def_proc_diskstat_filename);
    }

    if (unlikely(!__pf_diskstats)) {
        __pf_diskstats = procfile_open(__cfg_proc_diskstat_filename, " \t", PROCFILE_FLAG_DEFAULT);
        if (unlikely(!__pf_diskstats)) {
            error("[PLUGIN_PROC:proc_diskstats] Cannot open /proc/diskstats");
            return -1;
        }
        debug("[PLUGIN_PROC:proc_diskstats] opened '%s'", procfile_filename(__pf_diskstats));
    }

    __pf_diskstats = procfile_readall(__pf_diskstats);
    if (unlikely(!__pf_diskstats)) {
        error("Cannot read /proc/diskstats");
        return -1;
    }

    size_t pf_diskstats_lines = procfile_lines(__pf_diskstats), l;

    int64_t system_read_kb = 0, system_write_kb = 0;

    for (l = 0; l < pf_diskstats_lines; l++) {
        // 每一行都是一个磁盘设备
        char *dev_name;
        // 设备的主id，辅id
        uint32_t major, minor;

        size_t pf_diskstats_line_words = procfile_linewords(__pf_diskstats, l);
        if (unlikely(pf_diskstats_line_words < 14)) {
            // error("Cannot read /proc/diskstats: line %zu is too short.", l);
            continue;
        }

        major = str2uint32_t(procfile_lineword(__pf_diskstats, l, 0));
        minor = str2uint32_t(procfile_lineword(__pf_diskstats, l, 1));
        dev_name = procfile_lineword(__pf_diskstats, l, 2);

        struct io_device *dev = __get_device(dev_name, major, minor);
        struct io_stats  *curr_devstats = &dev->stats[dev->curr_stats_id];

        curr_devstats->rd_ios =
            str2uint64_t(procfile_lineword(__pf_diskstats, l, 3));   // prometheus.ReadIOs
        curr_devstats->wr_ios = str2uint64_t(procfile_lineword(__pf_diskstats, l, 7));   // WriteIOs

        curr_devstats->rd_merges =
            str2uint64_t(procfile_lineword(__pf_diskstats, l, 4));   // ReadMerges
        curr_devstats->wr_merges =
            str2uint64_t(procfile_lineword(__pf_diskstats, l, 8));   // WriteMerges

        curr_devstats->rd_sectors =
            str2uint64_t(procfile_lineword(__pf_diskstats, l, 5));   // ReadSectors
        curr_devstats->wr_sectors =
            str2uint64_t(procfile_lineword(__pf_diskstats, l, 9));   // WriteSectors

        curr_devstats->rd_ticks =
            str2uint64_t(procfile_lineword(__pf_diskstats, l, 6));   // ReadTicks
        curr_devstats->wr_ticks =
            str2uint64_t(procfile_lineword(__pf_diskstats, l, 10));   // WriteTicks

        curr_devstats->ios_pgr =
            str2uint64_t(procfile_lineword(__pf_diskstats, l, 11));   // IOsInProgress

        curr_devstats->ios_total_ticks =
            str2uint64_t(procfile_lineword(__pf_diskstats, l, 12));   // IOsTotalTicks

        curr_devstats->weighted_io_ticks =
            str2uint64_t(procfile_lineword(__pf_diskstats, l, 13));   // WeightedIOTicks

        if (unlikely(pf_diskstats_line_words > 13)) {
            curr_devstats->dc_ios =
                str2uint64_t(procfile_lineword(__pf_diskstats, l, 14));   // DiscardIOs
            curr_devstats->dc_merges =
                str2uint64_t(procfile_lineword(__pf_diskstats, l, 15));   // DiscardMerges
            curr_devstats->dc_sectors =
                str2uint64_t(procfile_lineword(__pf_diskstats, l, 16));   // DiscardSectors
            curr_devstats->dc_ticks =
                str2uint64_t(procfile_lineword(__pf_diskstats, l, 17));   // DiscardTicks
        }

        // debug(
        //     "\ndiskstats[%d:%d]: %s rd_ios=%lu, rd_merges=%lu, rd_sectors=%lu, rd_ticks=%lu,
        //     wr_ios=%lu, wr_merges=%lu, wr_sectors=%lu, wr_ticks=%lu, ios_pgr=%lu,
        //     ios_total_ticks=%lu, weighted_io_ticks=%lu, dc_ios=%lu, dc_merges=%lu,
        //     dc_sectors=%lu, dc_ticks=%lu", dev->major, dev->minor, dev->device_name,
        //     curr_devstats->rd_ios, curr_devstats->rd_merges, curr_devstats->rd_sectors,
        //     curr_devstats->rd_ticks, curr_devstats->wr_ios, curr_devstats->wr_merges,
        //     curr_devstats->wr_sectors, curr_devstats->wr_ticks, curr_devstats->ios_pgr,
        //     curr_devstats->ios_total_ticks, curr_devstats->weighted_io_ticks,
        //     curr_devstats->dc_ios, curr_devstats->dc_merges, curr_devstats->dc_sectors,
        //     curr_devstats->dc_ticks);

        if (unlikely(0 == dev->curr_stats_id && 0 == dev->prev_stats_id)) {
            dev->curr_stats_id = (dev->curr_stats_id + 1) % 2;
            // debug("disk[%d:%d]:'%s' prev_stats_id = %d, curr_stats_id = %d",
            //       major, minor, dev_name, dev->prev_stats_id,
            //       dev->curr_stats_id);
            // 设置指标
            prom_counter_add(__metric_node_disk_reads_completed_total,
                             (double)curr_devstats->rd_ios, (const char *[]){ dev_name });
            prom_counter_add(__metric_node_disk_reads_merged_total,
                             (double)curr_devstats->rd_merges, (const char *[]){ dev_name });
            prom_counter_add(__metric_node_disk_read_bytes_total,
                             (double)(curr_devstats->rd_sectors * dev->sector_size),
                             (const char *[]){ dev_name });
            // 毫秒转换为秒
            prom_counter_add(__metric_node_disk_read_time_seconds_total,
                             (double)curr_devstats->rd_ticks * 0.001, (const char *[]){ dev_name });

            prom_counter_add(__metric_node_disk_writes_completed_total,
                             (double)curr_devstats->wr_ios, (const char *[]){ dev_name });
            prom_counter_add(__metric_node_disk_writes_merged_total,
                             (double)curr_devstats->wr_merges, (const char *[]){ dev_name });
            prom_counter_add(__metric_node_disk_written_bytes_total,
                             (double)(curr_devstats->wr_sectors * dev->sector_size),
                             (const char *[]){ dev_name });
            // 毫秒转换为秒
            prom_counter_add(__metric_node_disk_write_time_seconds_total,
                             (double)curr_devstats->wr_ticks * 0.001, (const char *[]){ dev_name });

            prom_gauge_set(__metric_node_disk_io_now, (double)curr_devstats->ios_pgr,
                           (const char *[]){ dev_name });
            prom_counter_add(__metric_node_disk_io_time_seconds_total,
                             (double)curr_devstats->ios_total_ticks * 0.001,
                             (const char *[]){ dev_name });
            prom_counter_add(__metric_node_disk_io_time_weighted_seconds_total,
                             (double)curr_devstats->weighted_io_ticks * 0.001,
                             (const char *[]){ dev_name });

            prom_counter_add(__metric_node_disk_discards_completed_total,
                             (double)curr_devstats->dc_ios, (const char *[]){ dev_name });
            prom_counter_add(__metric_node_disk_discards_merged_total,
                             (double)curr_devstats->dc_merges, (const char *[]){ dev_name });
            prom_counter_add(__metric_node_discarded_sectors_total,
                             (double)curr_devstats->dc_sectors, (const char *[]){ dev_name });
            prom_counter_add(__metric_node_disk_discard_time_seconds_total,
                             (double)curr_devstats->dc_ticks * 0.001, (const char *[]){ dev_name });
            continue;
        }

        //--------------------------------------------------------------------
        // do performance metrics
        struct io_stats *prev_devstats = &dev->stats[dev->prev_stats_id];

        // 计算两次采集间隔时间，单位秒，dt單位是微秒所以要转换为秒
        double dt_sec = (double)dt / (double)USEC_PER_SEC;
        debug("[PLUGIN_PROC:proc_diskstats] disk[%d:%d]:'%s' dt_sec=%.2f, dt=%lu", major, minor,
              dev_name, dt_sec, dt);

        // 计算iostat输出的指标
        // TPS
        double tps_per_sec = (double)(curr_devstats->rd_ios - prev_devstats->rd_ios
                                      + curr_devstats->wr_ios - prev_devstats->wr_ios)
                             / dt_sec;
        // IO每秒读次数(r/s)
        double rd_ios_per_sec = ((double)(curr_devstats->rd_ios - prev_devstats->rd_ios)) / dt_sec;
        // IO每秒写次数(w/s)
        double wr_ios_per_sec = ((double)(curr_devstats->wr_ios - prev_devstats->wr_ios)) / dt_sec;

        // 每秒读取千字节数(rkB/s)
        double rd_kb_per_sec = ((double)(curr_devstats->rd_sectors - prev_devstats->rd_sectors))
                               * dev->sector_size / 1024 / dt_sec;
        // 每秒写入千字节数(wkB/s)
        double wr_kb_per_sec = ((double)(curr_devstats->wr_sectors - prev_devstats->wr_sectors))
                               * dev->sector_size / 1024 / dt_sec;

        // 每秒合并读次数 rrqm/s
        double rd_merges_per_sec =
            ((double)(curr_devstats->rd_merges - prev_devstats->rd_merges)) / dt_sec;
        // 每秒合并写次数 wrqm/s
        double wr_merges_per_sec =
            ((double)(curr_devstats->wr_merges - prev_devstats->wr_merges)) / dt_sec;

        // 每个读操作的耗时（毫秒）
        double r_await = (curr_devstats->rd_ios - prev_devstats->rd_ios) ?
                             (curr_devstats->rd_ticks - prev_devstats->rd_ticks)
                                 / ((double)(curr_devstats->rd_ios - prev_devstats->rd_ios)) :
                             0.0;

        // 每个写操作的耗时（毫秒）
        double w_await = (curr_devstats->wr_ios - prev_devstats->wr_ios) ?
                             (curr_devstats->wr_ticks - prev_devstats->wr_ticks)
                                 / ((double)(curr_devstats->wr_ios - prev_devstats->wr_ios)) :
                             0.0;

        uint64_t curr_nr_ios =
            curr_devstats->rd_ios + curr_devstats->wr_ios + curr_devstats->dc_ios;
        uint64_t prev_nr_ios =
            prev_devstats->rd_ios + prev_devstats->wr_ios + prev_devstats->dc_ios;
        // 	每個I/O平均所需的時間（毫秒）
        double await = (curr_nr_ios - prev_nr_ios) ?
                           ((curr_devstats->rd_ticks - prev_devstats->rd_ticks)
                            + (curr_devstats->wr_ticks - prev_devstats->wr_ticks)
                            + (curr_devstats->dc_ticks - prev_devstats->dc_ticks))
                               / ((double)(curr_nr_ios - prev_nr_ios)) :
                           0.0;

        // 平均未完成的I/O请求数量，时间单位秒，time_in_queue，time_in_queue是用当前的I/O数量（即字段#9
        // in-flight）乘以自然时间
        double aqu_sz =
            ((double)(curr_devstats->weighted_io_ticks - prev_devstats->weighted_io_ticks)) / dt_sec
            / 1000.0;

        // 该硬盘设备的繁忙比率
        double utils = ((double)(curr_devstats->ios_total_ticks - prev_devstats->ios_total_ticks))
                       / dt_sec / 10.0;

        // 每个I/O的平均扇区数
        double areq_sz = (curr_nr_ios - prev_nr_ios) ?
                             ((curr_devstats->rd_sectors - prev_devstats->rd_sectors)
                              + (curr_devstats->wr_sectors - prev_devstats->wr_sectors))
                                 / ((double)(curr_nr_ios - prev_nr_ios)) / 2.0 :
                             0.0;
        // rareq-sz (still in sectors, not kB)
        double rareq_sz = (curr_devstats->rd_ios - prev_devstats->rd_ios) ?
                              (curr_devstats->rd_sectors - prev_devstats->rd_sectors)
                                  / (double)(curr_devstats->rd_ios - prev_devstats->rd_ios) / 2.0 :
                              0.0;
        // wareq-sz
        double wareq_sz = (curr_devstats->wr_ios - prev_devstats->wr_ios) ?
                              (curr_devstats->wr_sectors - prev_devstats->wr_sectors)
                                  / (double)(curr_devstats->wr_ios - prev_devstats->wr_ios) / 2.0 :
                              0.0;

        if (dev->dev_tp == DISK_TYPE_PHYSICAL) {
            // count the global system disk I/O of physical disks
            system_read_kb += curr_devstats->rd_sectors / 2;
            system_write_kb += curr_devstats->wr_sectors / 2;

            debug("[PLUGIN_PROC:proc_diskstats] disk[%d:%d]:'%s' system_read_kb=%ld(Kb ), "
                  "system_write_kb=%ld(Kb), "
                  "rd_ios_per_sec=%.2f(r/s), "
                  "wr_ios_per_sec=%.2f(w/s), "
                  "rd_kb_per_sec=%.2f(rkB/s), wr_kb_per_sec=%.2f(wkB/s), "
                  "rd_merges_per_sec=%.2f(rrqm/s), "
                  "wr_merges_per_sec=%.2f(wrqm/s), r_await=%.2f(ms), w_await=%.2f(ms), "
                  "await=%.2f(ms), "
                  "aqu_sz=%.2f, areq_sz=%.2f, rareq_sz=%.2f, wareq_sz=%.2f， utils=%.2f\n",
                  major, minor, dev_name, system_read_kb, system_write_kb, rd_ios_per_sec,
                  wr_ios_per_sec, rd_kb_per_sec, wr_kb_per_sec, rd_merges_per_sec,
                  wr_merges_per_sec, r_await, w_await, await, aqu_sz, areq_sz, rareq_sz, wareq_sz,
                  utils);
        } else {
            debug("[PLUGIN_PROC:proc_diskstats] disk[%d:%d]:'%s' rd_ios_per_sec=%.2f(r/s), "
                  "wr_ios_per_sec=%.2f(w/s), "
                  "rd_kb_per_sec=%.2f(rkB/s), wr_kb_per_sec=%.2f(wkB/s), "
                  "rd_merges_per_sec=%.2f(rrqm/s), "
                  "wr_merges_per_sec=%.2f(wrqm/s), r_await=%.2f(ms), w_await=%.2f(ms), "
                  "await=%.2f(ms), "
                  "aqu_sz=%.2f, areq_sz=%.2f, rareq_sz=%.2f, wareq_sz=%.2f， utils=%.2f\n",
                  major, minor, dev_name, rd_ios_per_sec, wr_ios_per_sec, rd_kb_per_sec,
                  wr_kb_per_sec, rd_merges_per_sec, wr_merges_per_sec, r_await, w_await, await,
                  aqu_sz, areq_sz, rareq_sz, wareq_sz, utils);
        }

        // 设置指标
        prom_counter_add(__metric_node_disk_reads_completed_total,
                         (double)(curr_devstats->rd_ios - prev_devstats->rd_ios),
                         (const char *[]){ dev_name });
        prom_counter_add(__metric_node_disk_reads_merged_total,
                         (double)(curr_devstats->rd_merges - prev_devstats->rd_merges),
                         (const char *[]){ dev_name });
        prom_counter_add(
            __metric_node_disk_read_bytes_total,
            (double)((curr_devstats->rd_sectors - prev_devstats->rd_sectors) * dev->sector_size),
            (const char *[]){ dev_name });
        // 毫秒转换为秒
        prom_counter_add(__metric_node_disk_read_time_seconds_total,
                         (double)(curr_devstats->rd_ticks - prev_devstats->rd_ticks) * 0.001,
                         (const char *[]){ dev_name });

        prom_counter_add(__metric_node_disk_writes_completed_total,
                         (double)(curr_devstats->wr_ios - prev_devstats->wr_ios),
                         (const char *[]){ dev_name });
        prom_counter_add(__metric_node_disk_writes_merged_total,
                         (double)(curr_devstats->wr_merges - prev_devstats->wr_merges),
                         (const char *[]){ dev_name });
        prom_counter_add(
            __metric_node_disk_written_bytes_total,
            (double)((curr_devstats->wr_sectors - prev_devstats->wr_sectors) * dev->sector_size),
            (const char *[]){ dev_name });
        // 毫秒转换为秒
        prom_counter_add(__metric_node_disk_write_time_seconds_total,
                         (double)(curr_devstats->wr_ticks - prev_devstats->wr_ticks) * 0.001,
                         (const char *[]){ dev_name });

        prom_gauge_set(__metric_node_disk_io_now, (double)curr_devstats->ios_pgr,
                       (const char *[]){ dev_name });
        prom_counter_add(__metric_node_disk_io_time_seconds_total,
                         (double)(curr_devstats->ios_total_ticks - prev_devstats->ios_total_ticks)
                             * 0.001,
                         (const char *[]){ dev_name });
        prom_counter_add(
            __metric_node_disk_io_time_weighted_seconds_total,
            (double)(curr_devstats->weighted_io_ticks - prev_devstats->weighted_io_ticks) * 0.001,
            (const char *[]){ dev_name });

        prom_counter_add(__metric_node_disk_discards_completed_total,
                         (double)(curr_devstats->dc_ios - prev_devstats->dc_ios),
                         (const char *[]){ dev_name });
        prom_counter_add(__metric_node_disk_discards_merged_total,
                         (double)(curr_devstats->dc_merges - prev_devstats->dc_merges),
                         (const char *[]){ dev_name });
        prom_counter_add(__metric_node_discarded_sectors_total,
                         (double)(curr_devstats->dc_sectors - prev_devstats->dc_sectors),
                         (const char *[]){ dev_name });
        prom_counter_add(__metric_node_disk_discard_time_seconds_total,
                         (double)(curr_devstats->dc_ticks - prev_devstats->dc_ticks) * 0.001,
                         (const char *[]){ dev_name });

        prom_gauge_set(__metric_node_disk_iostat_tps, tps_per_sec, (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_read_requests_second, rd_ios_per_sec,
                       (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_write_requests_second, wr_ios_per_sec,
                       (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_read_kilobytes_second, rd_kb_per_sec,
                       (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_write_kilobytes_second, wr_kb_per_sec,
                       (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_read_requests_merged_second, rd_merges_per_sec,
                       (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_write_requests_merged_second, wr_merges_per_sec,
                       (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_r_await_msecs, r_await,
                       (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_w_await_msecs, w_await,
                       (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_await_msecs, await, (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_areq_sz, areq_sz, (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_rareq_sz, rareq_sz, (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_wareq_sz, wareq_sz, (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_aqu_sz, aqu_sz, (const char *[]){ dev_name });
        prom_gauge_set(__metric_node_disk_iostat_util, utils, (const char *[]){ dev_name });

        // --------------------------------------------------------------------
        dev->prev_stats_id = dev->curr_stats_id;
        dev->curr_stats_id = (dev->curr_stats_id + 1) % 2;
        // debug("disk[%d:%d]:'%s' prev_stats_id = %d, curr_stats_id = %d",
        //       major, minor, dev_name, dev->prev_stats_id, dev->curr_stats_id);
    }

    return 0;
}

void fini_collector_proc_diskstats() {
    if (likely(__pf_diskstats)) {
        procfile_close(__pf_diskstats);
        __pf_diskstats = NULL;
    }

    if (likely(__iodev_list)) {
        struct io_device *dev = __iodev_list;
        while (dev) {
            struct io_device *next = dev->next;
            free(dev);
            dev = next;
        }
    }
    debug("[PLUGIN_PROC:proc_diskstats] stopped");
}