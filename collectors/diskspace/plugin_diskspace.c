/*
 * @Author: CALM.WU
 * @Date: 2021-12-20 11:16:02
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-05-19 17:40:06
 */

#include "plugin_diskspace.h"

#include "prometheus-client-c/prom.h"

#include "routine.h"
#include "utils/clocks.h"
#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/log.h"
#include "utils/mountinfo.h"
#include "utils/simple_pattern.h"

#include "app_config/app_config.h"

#define DEFAULT_EXCLUDED_PATHS "/proc/* /sys/* /var/run/user/* /snap/* /var/lib/docker/* /dev/*"
#define DEFAULT_EXCLUDED_FILESYSTEMS                                     \
    "*gvfs *gluster* *s3fs *ipfs *davfs2 *httpfs *sshfs *gdfs *moosefs " \
    "fusectl autofs *gvfsd*"

static const char *__name = "PLUGIN_DISKSPACE";
static const char *__config_name = "collector_plugin_diskspace";

struct collector_diskspace {
    int32_t           exit_flag;
    pthread_t         thread_id;   // routine执行的线程ids
    int32_t           update_every;
    int32_t           check_for_new_mountinfos_every;
    SIMPLE_PATTERN   *excluded_mountpoints;
    SIMPLE_PATTERN   *excluded_filesystems;
    struct mountinfo *disk_mountinfo_root;
};

static struct collector_diskspace __collector_diskspace = {
    .exit_flag = 0,
    .thread_id = 0,
    .update_every = 1,
    .check_for_new_mountinfos_every = 15,
    .disk_mountinfo_root = NULL,
};

static prom_gauge_t *__metric_node_filesystem_avail_bytes = NULL,
                    *__metric_node_filesystem_free_bytes = NULL,
                    *__metric_node_filesystem_size_bytes = NULL,
                    *__metric_node_filesystem_files = NULL,
                    *__metric_node_filesystem_files_free = NULL,
                    *__metric_node_filesystem_readonly = NULL,
                    *__metric_node_filesystem_device_error = NULL;

__attribute__((constructor)) static void collector_diskspace_register_routine() {
    fprintf(stderr, "---register_collector_diskspace_routine---\n");
    struct xmonitor_static_routine *xsr =
        (struct xmonitor_static_routine *)calloc(1, sizeof(struct xmonitor_static_routine));
    xsr->name = __name;
    xsr->config_name = __config_name;   //配置文件中节点名
    xsr->enabled = 0;
    xsr->thread_id = &__collector_diskspace.thread_id;
    xsr->init_routine = diskspace_routine_init;
    xsr->start_routine = diskspace_routine_start;
    xsr->stop_routine = diskspace_routine_stop;
    register_xmonitor_static_routine(xsr);
}

static void __reload_mountinfo(int32_t force) {
    static time_t last_load = 0;

    time_t now = now_realtime_sec();

    if (force || now - last_load >= __collector_diskspace.check_for_new_mountinfos_every) {
        // 先释放
        mountinfo_free_all(__collector_diskspace.disk_mountinfo_root);
        __collector_diskspace.disk_mountinfo_root = mountinfo_read(0);
        last_load = now;
    }
}

static void __collector_diskspace_stats(struct mountinfo *mi, int32_t UNUSED(update_every)) {
    if (unlikely(
            simple_pattern_matches(__collector_diskspace.excluded_mountpoints, mi->mount_point))) {
        return;
    }

    if (unlikely(
            simple_pattern_matches(__collector_diskspace.excluded_filesystems, mi->filesystem))) {
        return;
    }

    // 设置指标
    unsigned long node_filesystem_size_bytes = 0;
    unsigned long node_filesystem_avail_bytes = 0;
    unsigned long node_filesystem_free_bytes = 0;
    unsigned long node_filesystem_reserve_root_size_bytes = 0;
    unsigned long node_filesystem_files = 0;
    unsigned long node_filesystem_files_free = 0;
    unsigned long node_filesystem_files_reserve_root = 0;
    unsigned long node_filesystem_files_used = 0;

    uint16_t node_filesystem_device_error = 0;
    uint16_t node_filesystem_readonly = 0;

    // statfs() is deprecated in favor of statvfs()
    struct statvfs vfs;
    if (unlikely(statvfs(mi->mount_point, &vfs) < 0)) {
        error("[PLUGIN_DISKSPACE] failed to statvfs() mount point '%s' (disk '%s', "
              "filesystem '%s', root '%s')",
              mi->mount_point, mi->mount_source, mi->filesystem ? mi->filesystem : "",
              mi->root ? mi->root : "");
        node_filesystem_device_error = 1;
    } else {
        // 基本文件系统块大小，磁盘的块大小是扇区
        // f_frsize The size in bytes of the minimum unit of allocation on this file
        // system
        fsblkcnt_t block_size = (vfs.f_frsize) ? vfs.f_frsize : vfs.f_bsize;
        // Size of fs in f_frsize units 文件系统数据块总数
        fsblkcnt_t block_total = vfs.f_blocks;
        // Number of free blocks 可用块数, root用户可用的数据块数
        fsblkcnt_t block_free = vfs.f_bfree;
        // 非超级用户可获取的块数 free blocks for unprivileged users
        fsblkcnt_t block_avail = vfs.f_bavail;
        // root用户保留块
        fsblkcnt_t block_reserve_root = block_free - block_avail;
        // fsblkcnt_t block_used = block_total - block_free;

        // inode数量
        node_filesystem_files = vfs.f_files;
        // Number of free inodes
        node_filesystem_files_free = vfs.f_ffree;
        // Number of free inodes for unprivileged users.
        fsblkcnt_t inode_avail = vfs.f_favail;
        // 为特权用户保留的inode数量
        node_filesystem_files_reserve_root = node_filesystem_files - inode_avail;
        // 已经使用的inode数量
        node_filesystem_files_used = node_filesystem_files - node_filesystem_files_free;

        // 设置指标
        node_filesystem_size_bytes = block_total * block_size / 1024;
        node_filesystem_avail_bytes = block_avail * block_size / 1024;
        node_filesystem_free_bytes = block_free * block_size / 1024;
        node_filesystem_reserve_root_size_bytes = block_reserve_root * block_size / 1024;

        if (mi->flags & MOUNTINFO_FLAG_READONLY) {
            node_filesystem_readonly = 1;
        }
    }

    prom_gauge_set(__metric_node_filesystem_size_bytes, (double)node_filesystem_size_bytes,
                   (const char *[]){ mi->mount_source, mi->filesystem, mi->mount_point });
    prom_gauge_set(__metric_node_filesystem_avail_bytes, (double)node_filesystem_avail_bytes,
                   (const char *[]){ mi->mount_source, mi->filesystem, mi->mount_point });
    prom_gauge_set(__metric_node_filesystem_free_bytes, (double)node_filesystem_free_bytes,
                   (const char *[]){ mi->mount_source, mi->filesystem, mi->mount_point });
    prom_gauge_set(__metric_node_filesystem_files, (double)node_filesystem_files,
                   (const char *[]){ mi->mount_source, mi->filesystem, mi->mount_point });
    prom_gauge_set(__metric_node_filesystem_files_free, (double)node_filesystem_files_free,
                   (const char *[]){ mi->mount_source, mi->filesystem, mi->mount_point });
    prom_gauge_set(__metric_node_filesystem_readonly, (double)node_filesystem_readonly,
                   (const char *[]){ mi->mount_source, mi->filesystem, mi->mount_point });
    prom_gauge_set(__metric_node_filesystem_device_error, (double)node_filesystem_device_error,
                   (const char *[]){ mi->mount_source, mi->filesystem, mi->mount_point });

    debug("[PLUGIN_DISKSPACE] Device:'%s', FileSystem:'%s' mounted on:'%s' size:%lu bytes "
          "avail:%lu bytes free:%lu bytes reserver_root:%lu bytes inode-total:%lu inode-used:%lu "
          "inode-free:%lu inode-reserver_root:%lu",
          mi->mount_source, mi->filesystem, mi->mount_point, node_filesystem_size_bytes,
          node_filesystem_avail_bytes, node_filesystem_free_bytes,
          node_filesystem_reserve_root_size_bytes, node_filesystem_files,
          node_filesystem_files_used, node_filesystem_free_bytes,
          node_filesystem_files_reserve_root);
}

int32_t diskspace_routine_init() {
    __collector_diskspace.update_every =
        appconfig_get_int("collector_plugin_diskspace.update_every", 1);
    __collector_diskspace.check_for_new_mountinfos_every =
        appconfig_get_int("collector_plugin_diskspace.check_for_new_mountinfos_every", 15);

    __collector_diskspace.excluded_mountpoints = simple_pattern_create(
        appconfig_get_str("collector_plugin_diskspace.exclude_mountpoints", DEFAULT_EXCLUDED_PATHS),
        NULL, SIMPLE_PATTERN_PREFIX);
    __collector_diskspace.excluded_filesystems =
        simple_pattern_create(appconfig_get_str("collector_plugin_diskspace.exclude_filesystems",
                                                DEFAULT_EXCLUDED_FILESYSTEMS),
                              NULL, SIMPLE_PATTERN_EXACT);

    debug("[%s] routine start, update_every: %d, check_for_new_mountinfos_every: %d", __name,
          __collector_diskspace.update_every, __collector_diskspace.check_for_new_mountinfos_every);

    __metric_node_filesystem_avail_bytes =
        prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_filesystem_avail_bytes",
            "node_filesystem_avail_bytes Filesystem space available to non-root users in bytes.", 3,
            (const char *[]){ "device", "fstype", "mountpoint" }));

    __metric_node_filesystem_free_bytes = prom_collector_registry_must_register_metric(
        prom_gauge_new("node_filesystem_free_bytes",
                       "node_filesystem_free_bytes Filesystem free space in bytes.", 3,
                       (const char *[]){ "device", "fstype", "mountpoint" }));

    __metric_node_filesystem_size_bytes =
        prom_collector_registry_must_register_metric(prom_gauge_new(
            "node_filesystem_size_bytes", "node_filesystem_size_bytes Filesystem size in bytes.", 3,
            (const char *[]){ "device", "fstype", "mountpoint" }));

    __metric_node_filesystem_files = prom_collector_registry_must_register_metric(prom_gauge_new(
        "node_filesystem_files", "node_filesystem_files Filesystem total file nodes.", 3,
        (const char *[]){ "device", "fstype", "mountpoint" }));

    __metric_node_filesystem_files_free = prom_collector_registry_must_register_metric(
        prom_gauge_new("node_filesystem_files_free",
                       "node_filesystem_files_free Filesystem total free file nodes.", 3,
                       (const char *[]){ "device", "fstype", "mountpoint" }));

    __metric_node_filesystem_readonly = prom_collector_registry_must_register_metric(prom_gauge_new(
        "node_filesystem_readonly", "node_filesystem_readonly Filesystem read-only status.", 3,
        (const char *[]){ "device", "fstype", "mountpoint" }));

    __metric_node_filesystem_device_error = prom_collector_registry_must_register_metric(
        prom_gauge_new("node_filesystem_device_error",
                       "node_filesystem_device_error Whether an error occurred while getting "
                       "statistics for the given device.",
                       3, (const char *[]){ "device", "fstype", "mountpoint" }));

    debug("[%s] routine init successed", __name);
    return 0;
}

void *diskspace_routine_start(void *UNUSED(arg)) {
    debug("[%s] routine, thread id: %lu start", __name, pthread_self());

    // 每次tick的时间间隔，转换为微秒
    usec_t step_usecs = __collector_diskspace.update_every * USEC_PER_SEC;

    struct heartbeat hb;
    heartbeat_init(&hb);

    while (!__collector_diskspace.exit_flag) {
        //等到下一个update周期
        heartbeat_next(&hb, step_usecs);

        if (__collector_diskspace.exit_flag) {
            break;
        }

        // 读取/proc/self/mountinfo，获取挂载文件系统信息
        // update by check_for_new_mountpoints_every
        __reload_mountinfo(0);

        //--------------------------------------------------------------------------------
        // disk space metrics
        debug("[%s] update filesystem metrics ====>", __name);
        struct mountinfo *mi;
        for (mi = __collector_diskspace.disk_mountinfo_root; mi; mi = mi->next) {
            if (unlikely(mi->flags & (MOUNTINFO_FLAG_IS_DUMMY | MOUNTINFO_FLAG_IS_BIND))) {
                // 忽略指定的文件系统和绑定文件系统
                continue;
            }

            __collector_diskspace_stats(mi, __collector_diskspace.update_every);

            if (unlikely(__collector_diskspace.exit_flag))
                break;
        }
    }

    debug("[%s] routine exit", __name);
    return NULL;
}

void diskspace_routine_stop() {
    __collector_diskspace.exit_flag = 1;
    pthread_join(__collector_diskspace.thread_id, NULL);

    if (likely(__collector_diskspace.disk_mountinfo_root)) {
        mountinfo_free_all(__collector_diskspace.disk_mountinfo_root);
        __collector_diskspace.disk_mountinfo_root = NULL;
    }

    if (__collector_diskspace.excluded_filesystems) {
        simple_pattern_free(__collector_diskspace.excluded_filesystems);
    }

    if (__collector_diskspace.excluded_mountpoints) {
        simple_pattern_free(__collector_diskspace.excluded_mountpoints);
    }

    debug("[%s] routine has completely stopped", __name);
    return;
}
