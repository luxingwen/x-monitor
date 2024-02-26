/*
 * @Author: CALM.WU
 * @Date: 2021-10-14 14:34:11
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2021-10-15 10:55:53
 */

#pragma once

#include "common.h"

typedef uint64_t usec_t;

// 纳秒
#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000ULL
#endif

// 微秒
#ifndef USEC_PER_SEC
#define USEC_PER_SEC 1000000ULL // 一秒钟的微秒数
#endif
#ifndef NSEC_PER_USEC
#define NSEC_PER_USEC 1000ULL // 一微秒的纳秒数
#endif

// 毫秒
#ifndef MSEC_PER_SEC
#define MSEC_PER_SEC 1000ULL
#endif
struct heartbeat {
    // 单调时间，字面意思是单调时间，实际上它指的是系统启动以后流逝的时间，
    // 这是由变量 jiffies 来记录的。系统每次启动时 jiffies 初始化为 0，每来一个
    // timer interrupt，jiffies 加 1，也就是说它代表系统启动后流逝的 tick 数
    usec_t monotonic;
    // 实际上就是指的是现实的时间，这是由变量 xtime 来记录的。系统每次启动时将
    // CMOS 上的 RTC 时间读入 xtime，这个值是"自 1970-01-01 起经历的秒数
    usec_t realtime;
};

// wall 时间
extern time_t now_realtime_sec();
// wall 时间，微秒
extern usec_t now_realtime_usec();

extern time_t now_monotonic_sec();
// jiffies 时间，微秒
extern usec_t now_monotonic_usec();

extern void heartbeat_init(struct heartbeat *hb);
// 返回自上一次心跳以来经过的时间（以微秒为单位）
extern usec_t heartbeat_next(struct heartbeat *hb, usec_t tick);

/*
CLOCK_REALTIME：可设定的系统级实时时钟。用于度量真实时间。
CLOCK_MONOTONIC ：不可设定的恒定态时钟。系统启动后就不会发生改变。
CLOCK_PROCESS_CPUTIME_ID：每进程CPU时间的时钟。测量调用进程所消耗的用户和系统CPU时间。
CLOCK_THREAD_CPUTIME_ID：每线程CPU时间的时钟。测量调用线程所消耗的用户和系统CPU时间。
CLOCK_MONOTONIC_RAW：提供了对纯基于硬件时间的访问。不受NTP时间调整的影响。
CLOCK_REALTIME_COARSE：类似于CLOCK_REALTIME，适用于希望以最小代价获取较低分辨率时间戳的程序。返回值分辨率为jiffy。
CLOCK_MONOTONIC_COARSE：类似于CLOCK_MONOTONIC，适用于希望以最小代价获取较低分辨率时间戳的程序。返回值分辨率为jiffy。
*/
extern void test_clock_monotonic_coarse();

// sleep for 微秒
extern int32_t sleep_usec(usec_t usec);

//
int64_t mktime64(const uint32_t year0, const uint32_t mon0, const uint32_t day,
                 const uint32_t hour, const uint32_t min, const uint32_t sec);