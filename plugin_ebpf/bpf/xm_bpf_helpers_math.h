/*
 * @Author: CALM.WU
 * @Date: 2022-10-31 15:27:19
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-10-31 15:32:01
 */

#pragma once

#include <bpf/bpf_helpers.h>

static inline __attribute__((always_inline)) __u64 __xm_log2(__u32 v) {
    __u32 r, shift;

    r = (v > 0xFFFF) << 4;
    v >>= r;
    shift = (v > 0xFF) << 3;
    v >>= shift;
    r |= shift;
    shift = (v > 0xF) << 2;
    v >>= shift;
    r |= shift;
    shift = (v > 0x3) << 1;
    v >>= shift;
    r |= shift;
    r |= (v >> 1);
    return r;
}
/*
log2l(1)=0
log2l(2)=1
log2l(7)=2
log2l(8)=3
*/
static inline __attribute__((always_inline)) __u64 __xm_log2l(__u64 v) {
    __u32 hi = v >> 32;
    if (hi)
        return __xm_log2(hi) + 32;
    else
        return __xm_log2(v);
}