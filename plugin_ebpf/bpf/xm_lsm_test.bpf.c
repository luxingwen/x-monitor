/*
 * @Author: CALM.WU
 * @Date: 2024-02-22 10:29:09
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-02-22 11:10:31
 */

#include <vmlinux.h>
#include "../bpf_and_user.h"
#include "xm_bpf_helpers_common.h"
#include "xm_bpf_helpers_maps.h"
#include "xm_bpf_helpers_net.h"

#define EPERM 1 /* Operation not permitted */
#define AF_INET 2

const volatile __u32 __blockip = 16909060; // 1.2.3.4

SEC("lsm/socket_connect")
__s32 BPF_PROG(restrict_connect, struct socket *sock, struct sockaddr *address,
               __s32 addrlen, __s32 ret) {
    if (address->sa_family != AF_INET) {
        // Only support ipv4
        return 0;
    }

    // Convert to IPv4 socket address
    struct sockaddr_in *addr = (struct sockaddr_in *)address;

    // Get destIP
    __u32 destip = addr->sin_addr.s_addr;
    // convert destIP to host endian
    __u32 h_destip = bpf_ntohl(destip);

    // Check if destIP is in block list
    if (destip == __blockip) {
        // Block
        bpf_printk("lsm_socket_connect block dest address:%d.%d.%d.%d",
                   (h_destip >> 24) & 0xff, (h_destip >> 16) & 0xff,
                   (h_destip >> 8) & 0xff, h_destip & 0xff);
        return -EPERM;
    }

    // print destIP
    bpf_printk("lsm_socket_connect pass dest address:%d.%d.%d.%d",
               (h_destip >> 24) & 0xff, (h_destip >> 16) & 0xff,
               (h_destip >> 8) & 0xff, h_destip & 0xff);

    return 0;
}
// ** GPL
char LICENSE[] SEC("license") = "Dual BSD/GPL";