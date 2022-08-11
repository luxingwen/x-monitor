/*
 * @Author: CALM.WU
 * @Date: 2022-08-11 11:01:21
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-08-11 14:18:23
 */

#include <vmlinux.h>
#include "xm_bpf_common.h"
#include "xm_bpf_parsing_helpers.h"

// 用来设置尾调函数句柄的map
struct {
    __uint(type, BPF_MAP_TYPE_PROG_ARRAY);
    __uint(key_size, sizeof(__u32));
    __uint(value_size, sizeof(__u32));
    __uint(max_entries, 8);
} tail_call_progs SEC(".maps");

#define PARSE_VLAN 1
#define PARSE_MPLS 2
#define PARSE_IP 3
#define PARSE_IPV6 4

struct flow_key_record {
    __be32 src;   // BIG ENDIAN，UDP/TCP/IP协议规定:把接收到的第一个字节当作高位字节看待
    __be32 dst;
    union {
        __be32 ports;
        __be16 port16[2];
    };
    __u32 ip_proto;
};

static inline void parse_eth_proto(struct __sk_buff *skb, __u16 eth_proto) {
    switch (eth_proto) {
    case ETH_P_8021Q:
    case ETH_P_8021AD:
        bpf_tail_call(skb, &tail_call_progs, PARSE_VLAN);
        break;
    case ETH_P_MPLS_UC:
    case ETH_P_MPLS_MC:
        bpf_tail_call(skb, &tail_call_progs, PARSE_MPLS);
        break;
    case ETH_P_IP:
        bpf_tail_call(skb, &tail_call_progs, PARSE_IP);
        break;
    case ETH_P_IPV6:
        bpf_tail_call(skb, &tail_call_progs, PARSE_IPV6);
        break;
    }
}

SEC("socket/1")
__s32 xmonitor_parse_vlan(struct __sk_buff *skb) {
    return 0;
}

SEC("socket/2")
__s32 xmonitor_parse_mpls(struct __sk_buff *skb) {
    return 0;
}

SEC("socket/3")
__s32 xmonitor_parse_ip(struct __sk_buff *skb) {
    return 0;
}

SEC("socket/4")
__s32 xmonitor_parse_ipv6(struct __sk_buff *skb) {
    return 0;
}

// 用来过滤链路层的数据包
SEC("socket/0")
__s32 xmonitor_main_socket_prog(struct __sk_buff *skb) {
    __u32 nhoff = ETH_HLEN;

    // 读取eth_hdr信息
    struct ethhdr eth_hdr;
    if (bpf_skb_load_bytes(skb, 0, &eth_hdr, sizeof(eth_hdr)) < 0) {
        bpf_printk("xmonitor read eth_hdr failed.");
    } else {
        __u16 eth_proto = bpf_ntohs(eth_hdr.h_proto);
        skb->cb[0] = nhoff;
        parse_eth_proto(skb, eth_proto);
    }
    return 0;
}
