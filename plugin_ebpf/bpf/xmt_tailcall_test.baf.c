/*
 * @Author: calmwu
 * @Date: 2023-04-22 16:19:46
 * @Last Modified by: calmwu
 * @Last Modified time: 2023-04-22 16:20:48
 */

#include <vmlinux.h>
#include <bpf/bpf_endian.h>
#include "xm_bpf_helpers_common.h"
#include "xm_xdp_stats_kern.h"
#include "xm_bpf_helpers_net.h"

struct bpf_map_def SEC("maps") prog_array_map = {
    .type = BPF_MAP_TYPE_PROG_ARRAY,
    .key_size = sizeof(int),
    .value_size = sizeof(int),
    .max_entries = 2,
};

SEC("socket_filter/first_prog")
int first_prog(struct __sk_buff *skb) {
    // Do some processing here...

    // Call the second eBPF program using tail call
    int key = 1;
    bpf_tail_call(skb, &prog_array_map, key);

    // Continue processing here...

    return 0;
}

SEC("socket_filter/second_prog")
int second_prog(struct __sk_buff *skb) {
    // Do some processing here...

    return 0;
}

char _license[] SEC("license") = "GPL";