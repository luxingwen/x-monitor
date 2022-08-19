/*
 * @Author: calmwu
 * @Date: 2022-05-08 14:44:34
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-05-19 17:01:43
 */

#pragma once

#include "prometheus-client-c/prom.h"

extern const char *__app_metric_minflt_help;
extern const char *__app_metric_vm_cminflt_help;
extern const char *__app_metric_majflt_help;
extern const char *__app_metric_vm_cmajflt_help;
extern const char *__app_metric_cpu_utime_help;
extern const char *__app_metric_cpu_stime_help;
extern const char *__app_metric_cpu_cutime_help;
extern const char *__app_metric_cpu_cstime_help;
extern const char *__app_metric_cpu_total_jiffies_help;
extern const char *__app_metric_num_threads_help;
extern const char *__app_metric_vmsize_kilobytes_help;
extern const char *__app_metric_vmrss_kilobytes_help;
extern const char *__app_metric_mem_rss_anon_kilobytes_help;
extern const char *__app_metric_mem_rss_file_kilobytes_help;
extern const char *__app_metric_mem_rss_shmem_kilobytes_help;
extern const char *__app_metric_mem_vmswap_kilobytes_help;
extern const char *__app_metric_mem_pss_kilobytes_help;
extern const char *__app_metric_mem_pss_anon_kilobytes_help;
extern const char *__app_metric_mem_pss_file_kilobytes_help;
extern const char *__app_metric_mem_pss_shmem_kilobytes_help;
extern const char *__app_metric_mem_uss_kilobytes_help;
extern const char *__app_metric_io_logical_bytes_read_help;
extern const char *__app_metric_io_logical_bytes_written_help;
extern const char *__app_metric_io_read_calls_help;
extern const char *__app_metric_io_write_calls_help;
extern const char *__app_metric_io_storage_bytes_read_help;
extern const char *__app_metric_io_storage_bytes_written_help;
extern const char *__app_metric_io_cancelled_write_bytes_help;
extern const char *__app_metric_open_fds_help;
extern const char *__app_metric_max_oom_score_help;
extern const char *__app_metric_max_oom_score_adj_help;
extern const char *__app_metric_nvcsw_help;
extern const char *__app_metric_nivcsw_help;

struct app_metrics {
    prom_gauge_t *metric_minflt;
    prom_gauge_t *metric_vm_cminflt;
    prom_gauge_t *metric_majflt;
    prom_gauge_t *metric_vm_cmajflt;
    prom_gauge_t *metric_cpu_utime;
    prom_gauge_t *metric_cpu_stime;
    prom_gauge_t *metric_cpu_cutime;
    prom_gauge_t *metric_cpu_cstime;
    prom_gauge_t *metric_cpu_total_jiffies;
    prom_gauge_t *metric_num_threads;
    prom_gauge_t *metric_vmsize_kilobytes;
    prom_gauge_t *metric_vmrss_kilobytes;
    prom_gauge_t *metric_mem_rss_anon_kilobytes;
    prom_gauge_t *metric_mem_rss_file_kilobytes;
    prom_gauge_t *metric_mem_rss_shmem_kilobytes;
    prom_gauge_t *metric_mem_vmswap_kilobytes;
    prom_gauge_t *metric_mem_pss_kilobytes;
    prom_gauge_t *metric_mem_pss_anon_kilobytes;
    prom_gauge_t *metric_mem_pss_file_kilobytes;
    prom_gauge_t *metric_mem_pss_shmem_kilobytes;
    prom_gauge_t *metric_mem_uss_kilobytes;
    prom_gauge_t *metric_io_logical_bytes_read;
    prom_gauge_t *metric_io_logical_bytes_written;
    prom_gauge_t *metric_io_read_calls;
    prom_gauge_t *metric_io_write_calls;
    prom_gauge_t *metric_io_storage_bytes_read;
    prom_gauge_t *metric_io_storage_bytes_written;
    prom_gauge_t *metric_io_cancelled_write_bytes;
    prom_gauge_t *metric_open_fds;
    prom_gauge_t *metric_max_oom_score;
    prom_gauge_t *metric_max_oom_score_adj;
    prom_gauge_t *metric_nvcsw;
    prom_gauge_t *metric_nivcsw;
};
