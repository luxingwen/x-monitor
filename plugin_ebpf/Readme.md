# x-monitor.eBPF

## 简介

本文通过表格的形式将x-montior.eBPF服务划分为各个子模块，细分出详细的功能能点，当前状态以及未来的目标

## 计划进度

| 子模块                    | 描述                                                         | 状态                                                |
| ------------------------- | ------------------------------------------------------------ | --------------------------------------------------- |
| x-monitor.eBPF服务框架    | 1：golang实现的web服务，服务框实现各个eBPF-Program的注册，注销，退出。<br />2：提供/metrics接口供Prometheus采集数据，服务导出Prometheus支持类型的指标。<br />3：配置管理，yaml格式，每个独立ebpf功能都有独立的配置项。<br />4：公共功能，事件中心，日志，信号，进程的pprof，内核/proc/kallsyms的读取。 | 完成。后续根据业务需求持续增强和优化。              |
| x-monitor.eBPF-Base       | 1：cilium-ebpf库的封装，支持kprobe，kretprobe，tracepoint，rawtracepoint，btf-rawtracepoint类型的eBPF program自动加载。<br />2：eBPF kernel Part通用编译。使用go generate自动化编译c代码<br />3：eBPF kernel Part公共代码，log、kernel struct/field read、helper_map、helper_net、helper_math、kernel struct/field CO-RE support。 | 完成。根据后续业务需求持续补充                      |
| x-monitor.eBPF-CacheStat  | 1：watch kprobe的add_to_page_cache_lru、mark_page_accessed、account_page_dirtied、mark_buffer_dirty记录进程在这三个函数的调用计数<br />2：使用BPF_HASH_MAP。<br />3：用户态代码读取HashMap数据，计算出pagecache.hits、pagecache.misses、pagecache.ratio，对外导出Prometheus Counter类型指标。 | 完成。                                              |
| x-monitor.eBPF-CPUSched   | 1：支持按os、namespace、cgroup、pid、pgid进行进程过滤<br />2：使用btf_rawtracepoint提升内核数据访问效率<br />3：记录task_struct插入运行队列的时间、调度上cpu的时间、离开cpu的时间。<br />4：使用ringbuf与用户态交互。<br />5：使用hash直方图来统计task_struct在cpu运行队列中等待时间的分布。<br />6：用户态代码输出CPU运行队列延时Prometheus直方图、<br />7：用户态代码输出超过阈值Off-cpu状态的进程，<br />8：用户态代码输出输出ProcessHand状态的进程。 | 完成。                                              |
| x-monitor.eBPF-ProcessVM  | 1：观察进程的private-anon和share地址空间的分配。<br />2：统计进程brk堆地址空间的分配。<br />3：进程私有地址空间、堆空间的释放<br />4：用户态代码到处Prometheus指标，显示过滤进程的private-anon+share地址空间大小，brk堆分配空间。 | 完成。                                              |
| x-monitor.eBPF-OOMKill    | 1：provides info on which processes got OOM killed。进程被kill的point得分，进程所在的cgroup，物理内存的详细占用。 | 1：阅读oom_kill.c代码<br />2：在编写kernel part代码 |
| x-monitor.eBPF.BioLatency | 1：将块设备的I/O延迟总结为直方图。                           | 待实现                                              |
| x-monitor.eBPF.nfs        | 1：对NFS进行监控                                             | 待实现                                              |
| x-monitor.eBPF.NAPI       | 1：NAPI机制在高速网络环境下的状态                            | 待实现                                              |

## 工具

| 名称           | 描述                                                         | 状态 |
| -------------- | ------------------------------------------------------------ | ---- |
| tools.parseelf | 1：解析elf文件，读取btf、section信息<br />2：方便对ob文件的解析 | 完成 |
| tools.trace    | 1：实现用户态指令，可复用kprobe逻辑<br />2：抓取指定系统调用参数的返回值，内核堆栈、用户态堆栈。<br />3：栈帧到代码的转换。 | 完成 |



## 测试

外网地址：[Prometheus Time Series Collection and Processing Server](http://159.27.191.120:9090/graph?g0.expr=process_address_space_privanon_share_pages&g0.tab=0&g0.stacked=0&g0.show_exemplars=0&g0.range_input=12h&g1.expr=cpu_schedule_runq_latency_bucket&g1.tab=0&g1.stacked=0&g1.show_exemplars=0&g1.range_input=1h&g2.expr=filesystem_pagecache_ratio&g2.tab=0&g2.stacked=0&g2.show_exemplars=0&g2.range_input=1h)

## 部署