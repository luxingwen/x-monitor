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

   注意，**在iptables中加上放通策略非常重要，否则是不通的**。

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

### 交互流程

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

#### calmveth1_peer-->bridge-->calmveth2_peer的转发

1. 这是calmveth1_peer的一个收包过程，veth收包逻辑在__netif_receive_skb_core函数中，应该调用dev->rx_handler函数将包传递给bridge

   ```
              <...>-1054946 [003] 4744773.708698: function:             __do_softirq
              <...>-1054946 [003] 4744773.708700: function:                irqtime_account_irq
              <...>-1054946 [003] 4744773.708701: function:                net_rx_action
              <...>-1054946 [003] 4744773.708703: function:                   __usecs_to_jiffies
              <...>-1054946 [003] 4744773.708704: function:                   __napi_poll
              <...>-1054946 [003] 4744773.708704: function:                      process_backlog
              <...>-1054946 [003] 4744773.708707: function:                         _raw_spin_lock
              <...>-1054946 [003] 4744773.708708: function:                         __netif_receive_skb
              <...>-1054946 [003] 4744773.708709: function:                         __netif_receive_skb_core
              <...>-1054946 [003] 4744773.708711: function:                            br_handle_frame
              <...>-1054946 [003] 4744773.708714: function:                               nf_hook_slow
   ```

2. 这是在bridge中的处理逻辑，可以看到br确定了转发端口后的发包逻辑，最终是发送到calmveth1的收包队列中，触发了中断

   ```
              <...>-1054946 [003] 4744773.710004: function:             br_dev_queue_push_xmit
              <...>-1054946 [003] 4744773.710119: function:                skb_push
              <...>-1054946 [003] 4744773.710120: function:                is_skb_forwardable
              <...>-1054946 [003] 4744773.710121: function:                dev_queue_xmit
              <...>-1054946 [003] 4744773.710122: function:                __dev_queue_xmit
              <...>-1054946 [003] 4744773.710124: function:                   netdev_pick_tx
              <...>-1054946 [003] 4744773.710126: function:                   validate_xmit_skb
              <...>-1054946 [003] 4744773.710127: function:                      netif_skb_features
              <...>-1054946 [003] 4744773.710129: function:                         passthru_features_check
              <...>-1054946 [003] 4744773.710130: function:                         skb_network_protocol
              <...>-1054946 [003] 4744773.710132: function:                      skb_csum_hwoffload_help
              <...>-1054946 [003] 4744773.710135: function:                   validate_xmit_xfrm
              <...>-1054946 [003] 4744773.710135: function:                   dev_hard_start_xmit
              <...>-1054946 [003] 4744773.710136: function:                      veth_xmit
              <...>-1054946 [003] 4744773.710139: function:                         skb_clone_tx_timestamp
              <...>-1054946 [003] 4744773.710141: function:                         __dev_forward_skb
              <...>-1054946 [003] 4744773.710142: function:                            is_skb_forwardable
              <...>-1054946 [003] 4744773.710144: function:                            skb_scrub_packet
              <...>-1054946 [003] 4744773.710145: function:                            eth_type_trans
              <...>-1054946 [003] 4744773.710147: function:                         netif_rx
              <...>-1054946 [003] 4744773.710148: function:                         netif_rx_internal
              <...>-1054946 [003] 4744773.710149: function:                            enqueue_to_backlog
              <...>-1054946 [003] 4744773.710150: function:                               _raw_spin_lock
   ```

3. 可以看到调用bridge处理函数br_handle_frame，通过上面的bt脚本，我们可以看到当br收到skb包后直行br_handle_frame的逻辑。

   skb从veth发送到peer

   ```
   veth_xmit skb=a90d6080 skblen=74 bytes skbdev=calmveth2 ===> peer_skbdev=calmveth2_peer
   	
   	ffffffffc06b5681 veth_xmit+1
   	ffffffff98fbffe6 dev_hard_start_xmit+150
   	ffffffff98fc0ab4 __dev_queue_xmit+2260
   	ffffffff99057ca6 ip_finish_output2+406
   	ffffffff99059700 ip_output+112
   	ffffffff9905912d __ip_queue_xmit+349
   	ffffffff99075662 __tcp_transmit_skb+1362
   	ffffffff990766c1 tcp_connect+2753
   	ffffffff9907d168 tcp_v4_connect+1112
   	ffffffff99098401 __inet_stream_connect+209
   	ffffffff990986d6 inet_stream_connect+54
   	ffffffff98f9b0ca __sys_connect+154
   	ffffffff98f9b116 __x64_sys_connect+22
   	ffffffff988042bb do_syscall_64+91
   	ffffffff992000ad entry_SYSCALL_64_after_hwframe+101
   ```

   __br_forward将skb的dev从calmveth1_peer改为calmveth2_peer，这里可以看到

   ```
   __br_forward skb=a90d6080 skblen=60 from calmveth2_peer ===> calmveth1_peer
   	
   	ffffffffc0654561 __br_forward+1
   	ffffffffc0656103 br_handle_frame_finish+339
   	ffffffffc069108b br_nf_hook_thresh+219
   	ffffffffc0691a10 br_nf_pre_routing_finish+304
   	ffffffffc0691fe5 br_nf_pre_routing+837
   	ffffffff99047204 nf_hook_slow+68
   	ffffffffc06565e1 br_handle_frame+497
   	ffffffff98fc153a __netif_receive_skb_core+714
   	ffffffff98fc38da process_backlog+170
   	ffffffff98fc326d __napi_poll+45
   	ffffffff98fc3763 net_rx_action+595
   	ffffffff994000d7 __softirqentry_text_start+215
   	ffffffff99200f4a do_softirq_own_stack+42
   	ffffffff988f226f do_softirq.part.14+79
   	ffffffff988f22cb __local_bh_enable_ip+75
   	ffffffff99057cba ip_finish_output2+426
   	ffffffff99059700 ip_output+112
   	ffffffff9905912d __ip_queue_xmit+349
   	ffffffff99075662 __tcp_transmit_skb+1362
   	ffffffff990766c1 tcp_connect+2753
   	ffffffff9907d168 tcp_v4_connect+1112
   	ffffffff99098401 __inet_stream_connect+209
   	ffffffff990986d6 inet_stream_connect+54
   	ffffffff98f9b0ca __sys_connect+154
   	ffffffff98f9b116 __x64_sys_connect+22
   	ffffffff988042bb do_syscall_64+91
   	ffffffff992000ad entry_SYSCALL_64_after_hwframe+101
   ```

   然后calmveth1_peer将包发送给calmveth1，下面这个堆栈是接着上面的。

   ```
   veth_xmit skb=a90d6080 skblen=74 bytes skbdev=calmveth1_peer ===> peer_skbdev=calmveth1
   	
   	ffffffffc06b5681 veth_xmit+1                    ****和发送一样，会调用enqueue_to_backlog，然后触发中断，让对端设备去读取
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
   	ffffffffc0654622 __br_forward+194                     ****这个函数主要负责将数据包从一个网桥端口转发到另一个网桥端口。当接收到的报文找到了转发表项后，就需要从指定的转发端口将报文转发出去。主要逻辑包括判断是否可以从端口中转发，根据需要判断是否需要复制一份报文再进行发送
   	ffffffffc0656103 br_handle_frame_finish+339                     ****函数主要负责决定将不同类别的数据包做不同的分发路径4。它会根据数据包的类型和目标地址，以及网桥和网桥端口的状态，来决定如何处理这个数据包2。例如，它会检查数据包是否是广播或多播，是否是本地地址，是否需要进行学习等
   	ffffffffc069108b br_nf_hook_thresh+219
   	ffffffffc0691a10 br_nf_pre_routing_finish+304
   	ffffffffc0691fe5 br_nf_pre_routing+837
   	ffffffff99047204 nf_hook_slow+68
   	ffffffffc06565e1 br_handle_frame+497                     ****调用calmveth2_peer的rx_handler函数
   	ffffffff98fc153a __netif_receive_skb_core+714                     ****在网卡calmveth2_peer的收包函数中
   	ffffffff98fc38da process_backlog+170
   	ffffffff98fc326d __napi_poll+45
   	ffffffff98fc3763 net_rx_action+595             **** 函数是软中断处理函数，它的作用是从poll_list中去除一个napi_struct结构，并调用process_backlog来处理输入队列中的skb。
   	ffffffff994000d7 __softirqentry_text_start+215
   	ffffffff99200f4a do_softirq_own_stack+42
   ```

   可以看到通过calmveth1_peer一个收包行为，在br中找到合适转发接口，设置目的dev为calmveth_peer1，然后调用calmveth_peer1的veth_xmit将数据发送给目的namespace空间中的calmveth2设备，这样就完成了链路层的转发

#### calmveth2收包

1. 这是设备calmveth2收包的内核函数踪迹，可见它是按照协议注册一层一层向上传递skb包了。

   ```
        netif_receive_skb_internal
           skb_defer_rx_timestamp
           __netif_receive_skb
           __netif_receive_skb_core
              ip_rcv
                 skb_clone
                    kmem_cache_alloc
                       should_failslab
                 __skb_clone
                    __copy_skb_header
                 consume_skb
                 pskb_trim_rcsum_slow
                 nf_hook_slow
                    ip_sabotage_in
                    ipv4_conntrack_defrag
                    ipv4_conntrack_in
                    nf_conntrack_in
                       get_l4proto
                       nf_ct_get_tuple
                       hash_conntrack_raw
                       __nf_conntrack_find_get
                       nf_conntrack_tcp_packet
                          nf_checksum
                          nf_ip_checksum
                          _raw_spin_lock_bh
                          tcp_in_window
                             nf_ct_seq_offset
                          _raw_spin_unlock_bh
                          __local_bh_enable_ip
                          __nf_ct_refresh_acct
                          nf_ct_acct_add
                    nf_nat_ipv4_in
                       nf_nat_ipv4_fn
                       nf_nat_inet_fn
                       nf_nat_packet
                 ip_rcv_finish
                    tcp_v4_early_demux
                       __inet_lookup_established
                          inet_ehashfn
                       ipv4_dst_check
                 ip_local_deliver
                    nf_hook_slow
                       nft_do_chain_ipv4
                          nft_do_chain
                             nft_update_chain_stats.isra.6
                       nf_nat_ipv4_fn
                       nf_nat_inet_fn
                       nf_nat_packet
                       nft_do_chain_ipv4
                          nft_do_chain
                             nft_update_chain_stats.isra.6
                       ipv4_helper
                       ipv4_confirm
                          nf_ct_deliver_cached_events
                    ip_local_deliver_finish
                       ip_protocol_deliver_rcu
                          raw_local_deliver
                          tcp_v4_rcv
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
                                   tcp_ack
                                      tcp_clean_rtx_queue
                                         tcp_rack_advance
                                         tcp_rate_skb_delivered
                                         __kfree_skb
                                            skb_release_all
                                               skb_release_head_state
                                            skb_release_data
                                            skb_free_head
                                            kfree
                                            __slab_free
                                         kfree_skbmem
                                         kmem_cache_free
                                            __slab_free
                                         tcp_chrono_stop
                                         tcp_ack_update_rtt.isra.52
                                            __usecs_to_jiffies
                                         bictcp_acked
                                      tcp_rack_update_reo_wnd
                                      tcp_schedule_loss_probe
                                      tcp_rearm_rto
                                      tcp_newly_delivered
                                      tcp_rate_gen
                                      bictcp_cong_avoid
                                      tcp_update_pacing_rate
                                   __kfree_skb
                                      skb_release_all
                                         skb_release_head_state
                                      skb_release_data
                                   kfree_skbmem
                                   kmem_cache_free
                                   tcp_check_space
              packet_rcv
                 consume_skb
   ```

   

veth收包过程比较复杂，核心函数是**__netif_receive_skb_core**，内部逻辑决定了包是否传递给tap（pcap），是否被dev注册的rx_handler函数处理，如何按注册的协议将skb向上传递。

net_rx_action：函数是软中断处理函数，它的作用是从poll_list中去除一个napi_struct结构，并调用process_backlog来处理输入队列中的skb。

process_backlog：是默认的skb包poll函数，从输入队列中取出一个skb，并调用netif_receive_skb函数，将其传递给协议栈。



