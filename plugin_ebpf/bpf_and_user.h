/*
 * @Author: CALM.WU
 * @Date: 2022-02-15 14:06:36
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-09-13 15:08:45
 */

#pragma once

#ifndef TASK_COMM_LEN
#define TASK_COMM_LEN 16
#endif

struct xdp_stats_datarec {
    __u64 rx_packets;
    __u64 rx_bytes;
};

struct xm_ebpf_event {
    pid_t pid;
    pid_t ppid;
    char  comm[TASK_COMM_LEN];
    __s32 err_code;   // 0 means success, otherwise means failed
};

#define XDP_UNKNOWN XDP_REDIRECT + 1
#ifndef XDP_ACTION_MAX
#define XDP_ACTION_MAX (XDP_UNKNOWN + 1)
#endif