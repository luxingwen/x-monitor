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

5. 搭建3号场景，如下命令

   ```
   brctl addbr calmbr
   ip link set dev calmbr up
   
   ip link add calmveth1 type veth peer name calmveth1_peer
   ip link add calmveth2 type veth peer name calmveth2_peer
   
   ip link set calmveth1 netns ns-calm1
   ip link set calmveth2 netns ns-calm2
   
   ip netns exec ns-calm1 ip addr add local 78.78.0.10/24 dev calmveth1
   ip netns exec ns-calm2 ip addr add local 78.78.0.11/24 dev calmveth2
   
   ip netns exec ns-calm1 ip link set up dev calmveth1
   ip netns exec ns-calm2 ip link set up dev calmveth2
   
   ip netns exec ns-calm1 ip link set up dev lo
   ip netns exec ns-calm2 ip link set up dev lo
   
   ip link set up dev calmveth1_peer
   ip link set up dev calmveth2_peer
   
   ip link set dev calmveth1_peer master calmbr
   ip link set dev calmveth2_peer master calmbr
   
   ip netns exec ns-calm1 ip route add default dev calmveth1_peer
   
   iptables -A FORWARD -i calmbr -j ACCEP
   ```

   注意，**在iptables中加上放通策略非常重要，否则是不通的**，因为我的主机是有容器环境的。

6. 联通测试

   - 在ns-calm1中启动服务

     ```
     root@Redhat8-01 /h/p/P/x/t/bin# ip netns exec ns-calm1 python3 ./hello_srv.py
     78.78.0.11 - - [01/Feb/2024 06:56:41] "GET / HTTP/1.1" 200 -
     ```

   - 在ns-calm2中启用curl

     ```
     root@Redhat8-01 /h/p/P/x/t/bin# ip netns exec ns-calm2 ./call_hello.sh 1
     发送请求 0
     hello
     ```

### SKB包的传递流程

#### calmveth1发出请求

1. call_hello.sh发送请求：trace-cmd record -p function -P 884548 ，跟踪python服务进程的详细堆栈，重点的函数用****进行了标识。

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

2. 编写bpftrace脚本[x-monitor/tools/bpftrace_shell/trace_veth.bt at main · wubo0067/x-monitor (github.com)](https://github.com/wubo0067/x-monitor/blob/main/tools/bpftrace_shell/trace_veth.bt)观察内核函数的变量值。

   ```
   veth_xmit skb=a90d6080 skblen=145 bytes skbdev=calmveth2 ===> peer_skbdev=calmveth2_peer
   	
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
   
   netif_rx_internal skbdev=calmveth2_peer received skb=a90d6080 skblen=131
   	
   	ffffffff98fbd371 netif_rx_internal+1                    ****将skb插入插入cpu队列中，触发软中断。然calmveth1_peer来读取
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

   可以看到veth_xmit会调用eth_type_trans，将skb的calmveth2改为calmveth2_peer
   
   ```
   __be16 eth_type_trans(struct sk_buff *skb, struct net_device *dev)
   {
   	unsigned short _service_access_point;
   	const unsigned short *sap;
   	const struct ethhdr *eth;
   	// 这里会改变 skb 的归属的 dev，如果是 veth，从 veth_xmit 过来，这里会将发送的 dev 改变为 peer_dev
   	skb->dev = dev;
   	skb_reset_mac_header(skb);
   ```

#### calmveth1_peer-->bridge-->calmveth2_peer的转发

1. 这是calmveth1_peer的一个收包过程，由软中断触发，veth收包逻辑在__netif_receive_skb_core函数中，应该调用dev->rx_handler函数将包传递给bridge

   ```
   static int __netif_receive_skb_core(struct sk_buff *skb, bool pfmemalloc)
   {
   	rx_handler = rcu_dereference(skb->dev->rx_handler);
   	if (rx_handler) {
   		if (pt_prev) {
   			ret = deliver_skb(skb, pt_prev, orig_dev);
   			pt_prev = NULL;
   		}
   		// 这里交给br_handle_frame处理
   		switch (rx_handler(&skb)) {
   		case RX_HANDLER_CONSUMED:
   			ret = NET_RX_SUCCESS;
   			goto out;
   }
   ```

2. 这是在bridge中的处理逻辑，可以看到br确定了转发端口后的发包逻辑，最终是发送到calmveth1的收包队列中，触发了中断

   ```
   veth_xmit skb=a90d6080 skblen=74 bytes skbdev=calmveth1_peer ===> peer_skbdev=calmveth1
   	
   	ffffffffc06b5681 veth_xmit+1                    ****和发送一样，会调用eth_type_trans将skb的dev从calmveth1_peer改为calmveth1，最后调用netif_rx_internal-->enqueue_to_backlog，然后触发中断，让calmveth1设备去读取
   	ffffffff98fbffe6 dev_hard_start_xmit+150
   	ffffffff98fc0ab4 __dev_queue_xmit+2260
   	ffffffffc06543a0 br_dev_queue_push_xmit+160
   	ffffffffc0690e3f br_nf_post_routing+735
   	ffffffff99047204 nf_hook_slow+68
   	ffffffffc065451f br_forward_finish+127
   	ffffffffc069108b br_nf_hook_thresh+219
   	ffffffffc06911a6 br_nf_forward_finish+262
   	ffffffffc069154c br_nf_forward_ip+780
   	ffffffff99047204 nf_hook_slow+68
   	ffffffffc0654622 __br_forward+194                     ****这个函数主要负责将数据包从一个网桥端口转发到另一个网桥端口。当接收到的报文找到了转发表项后，就需要从指定的转发端口将报文转发出去。主要逻辑包括判断是否可以从端口中转发，根据需要判断是否需要复制一份报文再进行发送，__br_forward将skb的dev从calmveth2_peer改为calmveth1_peer，精确的转到另一个veth。
   	ffffffffc0656103 br_handle_frame_finish+339                     ****函数主要负责决定将不同类别的数据包做不同的分发路径4。它会根据数据包的类型和目标地址，以及网桥和网桥端口的状态，来决定如何处理这个数据包2。例如，它会检查数据包是否是广播或多播，是否是本地地址，是否需要进行学习等
   	ffffffffc069108b br_nf_hook_thresh+219
   	ffffffffc0691a10 br_nf_pre_routing_finish+304
   	ffffffffc0691fe5 br_nf_pre_routing+837
   	ffffffff99047204 nf_hook_slow+68
   	ffffffffc06565e1 br_handle_frame+497                     ****调用calmveth2_peer的__netif_receive_skb_core函数中调用网桥的rx_handler函数
   	ffffffff98fc153a __netif_receive_skb_core+714                     ****在网卡calmveth2_peer的收包函数中
   	ffffffff98fc38da process_backlog+170
   	ffffffff98fc326d __napi_poll+45
   	ffffffff98fc3763 net_rx_action+595             **** 函数是软中断处理函数，它的作用是从poll_list中去除一个napi_struct结构，并调用process_backlog来处理输入队列中的skb。
   	ffffffff994000d7 __softirqentry_text_start+215
   	ffffffff99200f4a do_softirq_own_stack+42
   ```

#### calmveth2收包

收包路径是一个异步过程，内核从网卡收到数据包和调用recv/recvfrom/recvmsg是两个不同的过程。以下是内核从veth收包向上推送到socket的过程。

1. 这是bt脚本获取的收包下半部分的堆栈，由软中断触发，下面的堆栈是建立连接之后，calmveth1收到http请求的下半部分堆栈

   ```
   tcp_queue_rcv skb=a90d5300 skblen=149 skbdev='' dstip=78.78.0.10 from daddr=167792206
   	
   	ffffffff9906a611 tcp_queue_rcv+1             ****用于将接收到的数据包加入到接收队列receive_queue中，在Linux内核中，每一个网络数据包，都被切分为一个个的skb（Socket Buffer），这些skb先被内核接收，然后投递到对应的进程处理，进程把skb拷贝到本TCP连接的sk_receive_queue中，然后应答ACK，总的来说，receive_queue是用于存储接收到的网络数据包，等待上层应用程序进行处理的队列
   	ffffffff990719cd tcp_rcv_established+1149
   	ffffffff9907e05a tcp_v4_do_rcv+298
   	ffffffff990803b3 tcp_v4_rcv+2883
   	ffffffff99053b8c ip_protocol_deliver_rcu+44
   	ffffffff99053d7d ip_local_deliver_finish+77
   	ffffffff99053e70 ip_local_deliver+224
   	ffffffff990540fb ip_rcv+635
   	ffffffff98fc1e03 __netif_receive_skb_core+2963
   	ffffffff98fc38da process_backlog+170
   	ffffffff98fc326d __napi_poll+45
   	ffffffff98fc3763 net_rx_action+595
   	ffffffff994000d7 __softirqentry_text_start+215
   	ffffffff99200f4a do_softirq_own_stack+42
   	ffffffff988f226f do_softirq.part.14+79                    ****由calmveth1_peer触发的软中断到来，开始执行
   	ffffffff988f22cb __local_bh_enable_ip+75
   	ffffffff99057cba ip_finish_output2+426
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

2. 跟踪下ksoftirqd进程，来看看calmveth2收包的下半段(bottom half)的大致踪迹

   ```
   trace-cmd record -p function  -P 10 -P 18 -P 24 -P 3
   ```

   ```
   __netif_receive_skb
   __netif_receive_skb_core
      ip_rcv
         nf_hook_slow
            ipv4_conntrack_defrag
            ipv4_conntrack_in
            nf_conntrack_in
            nf_nat_ipv4_in
               nf_nat_ipv4_fn
               nf_nat_inet_fn
               nf_nat_packet
         ip_rcv_finish
         ip_local_deliver
            nf_hook_slow
               nf_nat_ipv4_fn
               nf_nat_inet_fn
               nf_nat_packet
               ipv4_helper
               ipv4_confirm
                  nf_ct_deliver_cached_events
            ip_local_deliver_finish
               ip_protocol_deliver_rcu
                  raw_local_deliver
                  tcp_v4_rcv
                     __inet_lookup_established
                        inet_ehashfn
                     tcp_v4_inbound_md5_hash
                        tcp_md5_do_lookup
                        tcp_parse_md5sig_option
                     sk_filter_trim_cap
                        security_sock_rcv_skb
                           selinux_socket_sock_rcv_skb
                              selinux_peerlbl_enabled
                                 netlbl_enabled
                           bpf_lsm_socket_sock_rcv_skb
                     tcp_v4_fill_cb
                     _raw_spin_lock
                     tcp_v4_do_rcv
                        ipv4_dst_check
                        tcp_rcv_established
                           tcp_mstamp_refresh
                              ktime_get
                           ktime_get_seconds
                           tcp_queue_rcv
                           tcp_event_data_recv
                           __tcp_ack_snd_check
                           tcp_send_delayed_ack
   ```

3. 问题

   1. 什么时候skb共享？skb可以被多个地方共享，例如被克隆时，__skb_clone会调用refcount_set(&n->users, 1)增加计数。skb需要被多个处理流程或者协议层处理时。skb->users就是一个计数来跟踪当前有多少个地方正在引用或者共享这个skb。

      **当skb->users的值大于1时，如果要进行修改，通常会使用skb_clone创建一个新的skb，然后对这个新的skb进行修改，以避免影响其他共享这个skb的地方**。

   2. 函数br_forward和br_pass_frame_up的区别：
   
      1.  `br_forward` 函数用于在桥接设备上执行数据帧的转发操作。具体而言，它用于将数据包从一个接口（端口）转发到另一个接口（端口）。
      2.  `br_pass_frame_up` 函数用于将接收到的数据帧向上传递到网络协议栈中。当一个数据帧到达桥接设备，但桥表中找不到匹配项时，需要将数据帧向上传递到网络协议栈中进行处理。
   

### 资料

1. [Tour Around Kernel Stack | Tao Wang (wtao0221.github.io)](https://wtao0221.github.io/2019/04/11/Tour-Around-Kernel-Stack/)



