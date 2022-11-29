/*
 * @Author: calmwu
 * @Date: 2022-05-08 19:10:57
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-05-19 17:03:27
 */

#include "app_metrics.h"

const char *__app_metric_minflt_help =
    "The number of minor faults the app has made which have not required "
    "loading a memory page from disk.";
const char *__app_metric_vm_cminflt_help =
    "The number of minor faults that the app's waited-for children have made.";
const char *__app_metric_majflt_help =
    "The number of major faults the app has made which have required loading a "
    "memory page from disk.";
const char *__app_metric_vm_cmajflt_help =
    "The number of major faults that the app's waited-for children have made.";
const char *__app_metric_cpu_utime_help =
    "The number of jiffies the app has been scheduled in user mode.";
const char *__app_metric_cpu_stime_help =
    "The number of jiffies the app has been scheduled in kernel mode.";
const char *__app_metric_cpu_cutime_help =
    "The number of jiffies the app's waited-for children have been scheduled in user mode.";
const char *__app_metric_cpu_cstime_help =
    "The number of jiffies the app's waited-for children have been scheduled in kernel mode.";
const char *__app_metric_cpu_total_jiffies_help =
    "The number of cpu jiffies the app has been scheduled";
const char *__app_metric_num_threads_help = "The number of threads the app has";
const char *__app_metric_vmsize_kilobytes_help = "The number of kilobytes of virtual memory";
const char *__app_metric_vmrss_kilobytes_help =
    "The number of kilobytes of resident memory, the value here is the sum of "
    "RssAnon, RssFile, and RssShmem.";
const char *__app_metric_mem_rss_anon_kilobytes_help =
    "The number of kilobytes of resident anonymous memory";
const char *__app_metric_mem_rss_file_kilobytes_help =
    "The number of kilobytes of resident file mappings";
const char *__app_metric_mem_rss_shmem_kilobytes_help =
    "The number of kilobytes of resident shared memory (includes System V "
    "shared memory, mappings from tmpfs(5), and shared anonymous mappings)";
const char *__app_metric_mem_vmswap_kilobytes_help =
    "The number of kilobytes of Swapped-out virtual memory size by anonymous private pages";
const char *__app_metric_mem_pss_kilobytes_help =
    "The number of kilobytes of Proportional Set memory size, It works exactly "
    "like RSS, but with the added difference of partitioning shared libraries";
const char *__app_metric_mem_pss_anon_kilobytes_help =
    "The number of kilobytes of Proportional Set memory size of anonymous private pages";
const char *__app_metric_mem_pss_file_kilobytes_help =
    "The number of kilobytes of Proportional Set memory size of file mappings";
const char *__app_metric_mem_pss_shmem_kilobytes_help =
    "The number of kilobytes of Proportional Set memory size of shared memory";
const char *__app_metric_mem_uss_kilobytes_help =
    "The number of kilobytes of Unique Set memory size, represents the private memory of a "
    "process.  it shows libraries and pages allocated only to this process";
const char *__app_metric_io_logical_bytes_read_help =
    "The number of bytes which this task has caused to be read from storage,  This is "
    "simply the sum of bytes which this process passed to read(2) and similar system calls";
const char *__app_metric_io_logical_bytes_written_help =
    "The number of bytes which this task has caused, or shall cause to be written to disk";
const char *__app_metric_io_read_calls_help =
    "Attempt to count the number of read I/O operations—that is, system calls "
    "such as read(2) and pread(2)";
const char *__app_metric_io_write_calls_help =
    "write syscalls Attempt to count the number of write I/O operations—that "
    "is, system calls such as write(2) and pwrite(2)";
const char *__app_metric_io_storage_bytes_read_help =
    "Attempt to count the number of bytes which this process really did cause to be "
    "fetched from the storage layer.  This is accurate for block-backed filesystems.";
const char *__app_metric_io_storage_bytes_written_help =
    "Attempt to count the number of bytes which this process caused to be sent "
    "to the storage layer.";
const char *__app_metric_io_cancelled_write_bytes_help =
    "this field represents the number of bytes which this process caused to not "
    "happen, by truncating pagecache.";
const char *__app_metric_open_fds_help = "The number of open file descriptors";
const char *__app_metric_sock_fds_help = "Statistics of socket type, status and quantity";
const char *__app_metric_max_oom_score_help =
    "This file displays the current score that the kernel gives to this process for the purpose of "
    "selecting a process for the OOM-killer.  A higher score means that the process is more likely "
    "to be selected by the OOM-killer.";
const char *__app_metric_max_oom_score_adj_help =
    "This file can be used to adjust the badness heuristic used to select which process gets "
    "killed in out-of-memory conditions.";

const char *__app_metric_nvcsw_help = "The number of voluntary context switches";
const char *__app_metric_nivcsw_help = "The number of involuntary context switches";