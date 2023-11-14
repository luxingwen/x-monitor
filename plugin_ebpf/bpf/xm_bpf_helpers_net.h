/*
 * @Author: CALM.WU
 * @Date: 2022-02-10 16:37:40
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-11-14 15:24:23
 */

#include <vmlinux.h>
#include <bpf/bpf_endian.h>

#ifndef ETH_HLEN
#define ETH_HLEN 14 /* Total octets in header.	 */
#endif

#ifndef ETH_P_8021Q
#define ETH_P_8021Q 0x8100 /* 802.1Q VLAN Extended Header  */
#endif

#ifndef ETH_P_8021AD
#define ETH_P_8021AD 0x88A8 /* 802.1ad Service VLAN */
#endif

#ifndef ETH_P_IP
#define ETH_P_IP 0x0800 /* Internet Protocol packet */
#endif

#ifndef ETH_P_IPV6
#define ETH_P_IPV6 0x86DD /* Internet Protocol Version 6 packet */
#endif

#define ETH_P_MPLS_UC 0x8847 /* MPLS Unicast traffic		*/
#define ETH_P_MPLS_MC 0x8848 /* MPLS Multicast traffic	*/

#define cursor_advance(_cursor, _len) \
    ({                                \
        void *_tmp = _cursor;         \
        _cursor += _len;              \
        _tmp;                         \
    })

struct hdr_cursor {
    void *pos;
};

static __s32 __xm_proto_is_vlan(__u16 h_proto) {
    return !!(h_proto == bpf_htons(ETH_P_8021Q)
              || h_proto == bpf_htons(ETH_P_8021AD));
}

// 返回 IP 包承载的具体协议类型，tcp、udp、icmp 等
static __u8 __xm_parse_ip4hdr(struct hdr_cursor *nh, void *data_end,
                              struct iphdr **iphdr) {
    struct iphdr *iph = nh->pos;
    int hdrsize;

    if ((void *)(iph + 1) > data_end)
        return -1;

    hdrsize = iph->ihl * 4;
    /* Sanity check packet field is valid */
    if (hdrsize < sizeof(*iph))
        return -1;

    /* Variable-length IPv4 header, need to use byte-based arithmetic */
    if (nh->pos + hdrsize > data_end)
        return -1;

    nh->pos += hdrsize;
    *iphdr = iph;

    return iph->protocol;
}

static __u8 __xm_parse_ip6hdr(struct hdr_cursor *nh, void *data_end,
                              struct ipv6hdr **ip6hdr) {
    struct ipv6hdr *ip6h = nh->pos;

    /* Pointer-arithmetic bounds check; pointer +1 points to after end of
     * thing being pointed to. We will be using this style in the remainder
     * of the tutorial.
     */
    if ((void *)(ip6h + 1) > data_end)
        return -1;

    nh->pos = ip6h + 1;
    *ip6hdr = ip6h;

    return ip6h->nexthdr;
}

static __s32 __get_dport(void *trans_data, void *data_end, __u8 protocol) {
    struct tcphdr *th;
    struct udphdr *uh;

    switch (protocol) {
    case IPPROTO_TCP:
        th = (struct tcphdr *)trans_data;
        if ((void *)(th + 1) > data_end)
            return -1;
        return th->dest;
    case IPPROTO_UDP:
        uh = (struct udphdr *)trans_data;
        if ((void *)(uh + 1) > data_end)
            return -1;
        return uh->dest;
    default:
        return 0;
    }
}

// 解析包头得到 ethertype
static bool __xm_parse_eth(struct ethhdr *eth, void *data_end,
                           __u16 *eth_type) {
    __u64 offset;

    offset = sizeof(*eth);
    if ((void *)eth + offset > data_end)
        return false;
    *eth_type = eth->h_proto;
    return true;
}
