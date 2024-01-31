# 跟踪skb在veths设备的传输过程

最近在生产rhel7上遇到一个问题，向veth一端发送的数据另一端的应用接收后是错乱的。以此记录下定位问题过程中分析的skb在veth设备中skb包的转发过程。

#### veth几种场景（同一个主机）

1. veth1 ------ veth2 在同一个ns
2. veth1 ------ veth2 在不同的ns
3. veth1 ------ veth2 ------ **bridge** ------ veth3 ------ veth4

![image-20240131111622151](./img/veth)

在不同的场景下，skb包的传输过程是不同的，这对加上理解veth工作流程是个递进的过程。

本文重点去考虑场景3。简单介绍下场景1，一开始会以为当从veth1发送数据，在链路层会调用veth_xmit发送到peer的veth，其实不然，在同一个机器上走的loopback_xmit.

#### 环境和工具

由于需要跟踪内核运行时的逻辑，主要是基于kprobe来实现。

1. 安装debug版本的内核，由于veth是一个内核模块，也需要安装debug版本

   ![image-20240131111830022](./img/debug-kernel.img)

2. 安装kprobe，ftrace的使用工具。perf、trace-cmd、bpftrace

3. 一个内核函数反汇编的脚本，使用objdump -d -S，[x-monitor/tools/bin/vmlinux_disassembler.sh at main · wubo0067/x-monitor (github.com)](https://github.com/wubo0067/x-monitor/blob/main/tools/bin/vmlinux_disassembler.sh)，有个这个可以进行地址和代码的对照

4. 编写一个python http服务，客户端用个curl命令指定veth接口进行访问。

#### 发包流程

1. 命令：trace-cmd record -p function -P 884548 ，跟踪python服务进程的详细堆栈，重点的函数用****进行了标识。

   ```
   __x64_sys_sendto
      __sys_sendto                     ****
         sockfd_lookup_light
            __fdget
            __fget_light
         sock_sendmsg                     ****
            security_socket_sendmsg
               selinux_socket_sendmsg
               sock_has_perm
                  avc_has_perm
               bpf_lsm_socket_sendmsg
            inet_sendmsg                     ****
               inet_send_prepare
            tcp_sendmsg                     ****
               lock_sock_nested
                  _cond_resched
                     rcu_all_qs
                  _raw_spin_lock_bh
               __local_bh_enable_ip
               tcp_sendmsg_locked                     ****
                  tcp_rate_check_app_limited
                  tcp_send_mss
                     tcp_current_mss
                        ipv4_mtu
                        tcp_established_options
                  sk_stream_alloc_skb                     ****
                     __alloc_skb
                        kmem_cache_alloc_node
                           _cond_resched
                              rcu_all_qs
                           should_failslab
                        __kmalloc_reserve.isra.54
                           __kmalloc_node_track_caller
                              kmalloc_slab
                  tcp_tx_timestamp                     ****
                  tcp_push                     ****
                  __tcp_push_pending_frames		   
                     tcp_write_xmit                     ****
                        ktime_get
                        tcp_small_queue_check.isra.39
                        __tcp_transmit_skb
                           skb_clone
                           __skb_clone
                              __copy_skb_header
                           tcp_established_options
                           skb_push
                           __tcp_select_window
                           tcp_options_write
                           bpf_skops_write_hdr_opt.isra.45
                           tcp_v4_send_check
                           __tcp_v4_send_check
                           bictcp_cwnd_event
                           __ip_queue_xmit                     ****，这里进入ip层
                              __sk_dst_check
                                 ipv4_dst_check
                              skb_push
                              ip_copy_addrs
                              ip_local_out
                                 __ip_local_out
                                    ip_send_check
                                    nf_hook_slow
                                       selinux_ipv4_output
                                          netlbl_enabled
                              ip_output
                                 nf_hook_slow
                                    selinux_ipv4_postroute
                                    selinux_ip_postroute
                                       selinux_peerlbl_enabled
                                          netlbl_enabled
                                 ip_finish_output	
                                 ip_finish_output2
                                    neigh_resolve_output
                                       eth_header	
                                          skb_push
                                    dev_queue_xmit                     ****，进入链路层，网卡驱动发送逻辑
                                    __dev_queue_xmit
                                       netdev_pick_tx
                                       validate_xmit_skb
                                          netif_skb_features
                                             passthru_features_check
                                             skb_network_protocol
                                          skb_csum_hwoffload_help
                                       validate_xmit_xfrm
                                       dev_hard_start_xmit
                                          veth_xmit                     ****，veth的发送函数
                                             skb_clone_tx_timestamp
                                             __dev_forward_skb
                                                is_skb_forwardable
                                                skb_scrub_packet
                                                eth_type_trans                     ****，这里会将skb的dev换成peer。
                                             netif_rx
                                             netif_rx_internal
                                                enqueue_to_backlog                     ****，将skb包放入cpu的输入队列中，
                                                   _raw_spin_lock
                                                   __raise_softirq_irqoff
                                       __local_bh_enable_ip
                                    __local_bh_enable_ip
                                       do_softirq.part.14                     ****，触发一个中断，这会触发另外一个设备的读取
   ```

   netif_rx：函数是由非NAPI网络设备驱动程序在接收中断将数据包从设备缓冲区拷贝到内核空间后调用，它主要的人数是吧数据帧添加到CPU的输入队列中，并标记中断来处理后续上传给TCP/IP协议栈。

   enqueue_to_backlog：函数是netif_rx的内部函数，将skb放入CPU的私有输入队列。

2. 编写bpftrace脚本观察内核函数的变量值。

   ```
   veth_xmit skblen=144 bytes skbdev=calmveth0 ===> peer_skbdev=calmveth1
   	
   	ffffffffc06b5681 veth_xmit+1
   	ffffffff98fbffe6 dev_hard_start_xmit+150
   	ffffffff98fc0ab4 __dev_queue_xmit+2260
   	ffffffff99057ca6 ip_finish_output2+406
   	ffffffff99059700 ip_output+112
   	ffffffff9905912d __ip_queue_xmit+349
   	ffffffff99075662 __tcp_transmit_skb+1362
   	ffffffff99076f85 tcp_write_xmit+1077
   	ffffffff99077e32 __tcp_push_pending_frames+50
   	ffffffff99063d38 tcp_sendmsg_locked+3128
   	ffffffff99063ec7 tcp_sendmsg+39
   	ffffffff98f9a13e sock_sendmsg+62
   	ffffffff98f9b43e __sys_sendto+238
   	ffffffff98f9b4d4 __x64_sys_sendto+36
   	ffffffff988042bb do_syscall_64+91
   	ffffffff992000ad entry_SYSCALL_64_after_hwframe+101
   
   netif_rx_internal skbdev=calmveth1 received skblen=130
   	
   	ffffffff98fbd371 netif_rx_internal+1
   	ffffffffc06b5830 veth_xmit+432
   	ffffffff98fbffe6 dev_hard_start_xmit+150
   	ffffffff98fc0ab4 __dev_queue_xmit+2260
   	ffffffff99057ca6 ip_finish_output2+406
   	ffffffff99059700 ip_output+112
   	ffffffff9905912d __ip_queue_xmit+349
   	ffffffff99075662 __tcp_transmit_skb+1362
   	ffffffff99076f85 tcp_write_xmit+1077
   	ffffffff99077e32 __tcp_push_pending_frames+50
   	ffffffff99063d38 tcp_sendmsg_locked+3128
   	ffffffff99063ec7 tcp_sendmsg+39
   	ffffffff98f9a13e sock_sendmsg+62
   	ffffffff98f9b43e __sys_sendto+238
   	ffffffff98f9b4d4 __x64_sys_sendto+36
   	ffffffff988042bb do_syscall_64+91
   	ffffffff992000ad entry_SYSCALL_64_after_hwframe+101
   
   ```

   可以看到veth_xmit后面，skb的dev从veth一端改为另一端了。

   #### 收包流程

   net_rx_action：函数是软中断处理函数，它的作用是从poll_list中去除一个napi_struct结构，并调用process_backlog来处理输入队列中的skb。

   process_backlog：是默认的skb包poll函数，从输入队列中取出一个skb，并调用netif_receive_skb函数，将其传递给协议栈。



