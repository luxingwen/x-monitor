# CGroupV1

## CPU资源限制
### CPUACCT控制器

自动生成控制组中任务对CPU资源使用情况的报告。

#### cpu.shares，按权重比例设定CPU的分配

shares用来设置CPU的相对值，必须大于等于2，最后除以权重综合，算出相对比例，按比例分配CPU时间。该指是针对**所有的CPU**。

cgroup A设置为100, cgroup B设置为300，那么cgroupA中的任务运行25%的CPU时间。对于一个**四核CPU**的系统来说，cgroup A中的任务可以100%占有某一个CPU，这个比例是相对整体的一个值。

shares值有如下特点：

1. 如果A不忙，没有使用25%的时间，那么剩余的CPU时间将会被系统分配给B，**即B的CPU使用率可以超过75%**。
2. 在闲的时候，shares基本上不起作用，只有在CPU忙的时候起作用。
3. 由于shares是一个绝对值，需要和其它cgroup的值进行比较才能得到自己的相对限额，如果在一个部署**很多容器的机器上**，cgroup的数量是变化的，所以这个限额也是变化的。

cpu.shares是几个控制组之间的CPU分配比例，而且一定要到整个节点中所有的CPU都跑满的时候，它才发挥作用。

#### cpu.cfs_quota_us和cpu.cfs_period_us，设置CPU使用周期和使用上限。

cpu调度策略有两种：

- 完全公平调度（Completely Fair Scheduler，CFS），按限额和比例分配两种方式进行资源限制。
- 实时调度（Real-TimeScheduler），针对实时任务按周期分配固定的运行时间。

cfs_period_us：用来配置时间周期长度，单位：**微秒**。取值范围1毫秒(1000ms)~1秒(1000000ms)。它是CFS算法的一个调度周期，一般是100000。

cfs_quota_us：配置当前cgroup在设置的周期长度内所能使用的cpu时间数，单位：微秒，取值大于1ms即可，如果值为-1（默认值），表示不受CPU时间的限制。它表示CFS算法中，在一个调度周期里这个控制组被允许的运行时间。

两个文件配合起来可以以**绝对比例限制cgroup的cpu的使用上限**。举例：

```
1.限制只能使用1个CPU（每250ms能使用250ms的CPU时间）
    # echo 250000 > cpu.cfs_quota_us /* quota = 250ms */
    # echo 250000 > cpu.cfs_period_us /* period = 250ms */

2.限制使用2个CPU（内核）（每500ms能使用1000ms的CPU时间，即使用两个内核）
    # echo 1000000 > cpu.cfs_quota_us /* quota = 1000ms */
    # echo 500000 > cpu.cfs_period_us /* period = 500ms */

3.限制使用1个CPU的20%（每50ms能使用10ms的CPU时间，即使用一个CPU核心的20%）
    # echo 10000 > cpu.cfs_quota_us /* quota = 10ms */
    # echo 50000 > cpu.cfs_period_us /* period = 50ms */
```

#### cpu.shares和cpu.cfs_quota_us、cpu.cfs_period_us一起使用

三者合起来一起来综合判断，决定了能获得CPU资源上限，上限分为两种情况：

1. 当主机上有很多cgroup，节点上CPU已经完全占满，这时控制器获得的CPU资源由shares来决定。
2. 当主机比较闲，控制器中的进程获得cpu资源上限有quota/period来决定。

**结合来看cpu.cfs_quota_us决定了CPU资源的上限，cpu.shares决定了在繁忙机器上能使用CPU的下限**。

举例，有两个相同的层级的cgroup，bar和baz，在一个只有1core的节点，cpu.cfs_period_us都设置为100ms，cpu.share都设置为1024.

1. 当bar和baz的cpu.cfs_quota_us都设置超过50ms，例如75ms，这意味两个cgroup的cpu使用率之和大于1core，这时两个cgroup会平均分配cpu资源，每个都是50ms，50%的core。

2. 如果cpu.cfs_quota_us都设置为25ms，小于50ms，这意味两个cgroup的cpu使用率之和小于1core，这时他们的也会1:1的分配cpu，不过确切是各占25%的core。

3. 如果bar的quota设置为25ms，而baz设置为75ms，两者还是有相同的shares值。这是bar的cpu上限是25ms，如果是period是100ms，baz还会按照shares的1:1分配到25ms吗？如果是，那么剩余的50ms放到哪里了？

   ```
   Because the CFS does not demand equal usage of CPU, it is hard to predict how much CPU time a cgroup will be allowed to utilize. When tasks in one cgroup are idle and are not using any CPU time, the leftover time is collected in a global pool of unused CPU cycles. Other cgroups are allowed to borrow CPU cycles from this pool.
   ```

   实际是baz可以消费75ms的CPU的。

总结这个场景，两者是一起工作的。

```
cpu.share and cpu.cfs_quota_us are working together.

Given a total cpu quota, we should firstly distribute the cpu.share of each cgroup. Then find the cgroups whose exact quota exceeds their cpu.cfs_quota_us, find all such cgroups and keep their quota as their cpu.cfs_quota_us, and collect the exceeded part as unused cpu pool. Distribute these unused cpu pool among other cgroups by cpu.share again, and iterate as above, until no cgroup is exceeding the upper limit.
```

#### cpu资源报告

提供了CPU资源用量的统计

- cpuacct.usage，报告一个`cgroup`中所有任务（包括其子孙层级中的所有任务）使用`CPU`的总时间（纳秒）,该文件时可以写入`0`值的，用来进行重置统计信息。
- cpuacct.usage_user，报告一个`cgroup`中所有任务（包括其子孙层级中的所有任务）使用用户态`CPU`的总时间（纳秒）。
- cpuacct.usage_sys，报告一个`cgroup`中所有任务（包括其子孙层级中的所有任务）使用内核态`CPU`的总时间（纳秒）。
- cpuacct.usage_percpu，报告一个`cgroup`中所有任务（包括其子孙层级中的所有任务）在每个`CPU`使用`CPU`的时间（纳秒）。
- cpuacct.usage_percpu_user，报告一个`cgroup`中所有任务（包括其子孙层级中的所有任务）在每个`CPU`上使用用户态`CPU`的时间（纳秒）。
- cpuacct.usage_percpu_sys，报告一个`cgroup`中所有任务（包括其子孙层级中的所有任务）在每个`CPU`上使用内核态`CPU`的时间（纳秒）。
- cpuacct.usage_all，详细输出文件`cpuacct.usage_percpu_user`和`cpuacct.usage_percpu_sys`的内容。
- cpuacct.stat，报告cgroup的所有任务（包括其子孙层级中的所有任务）使用的用户和系统CPU时间，单位是USER_HZ（100ms）
- cpu.stat
  - nr_periods：表示过去了多少个cpu.cfs_period_us里面配置的时间周期。
  - nr_throttled：在上面这些周期中，有多少次是受到了限制（即cgroup中的进程在指定的时间周期中用光了它的配额）。
  - throttled_time：cgroup中的进程被限制使用CPU持续了多长时间，单位是ns。

## 内存资源限制

当限制内存时，我们最好想清楚如果内存超限了发生什么？该如何处理？业务是否可以接受这样的状态？

### 内存控制能限制什么

- 限制cgroup中所有进程所能使用的物理内存总量。
- 限制cgroup中所有能使用的物理内存+交换空间总量（CONFIG_MEMCG_SWAP），一般在server不开启swap空间。

### Memory Cgroup主要文件

![memory_cgroup_file](./img/memory_cgroup_file.jpg)

memory.stat说明

![cgroup_memory_stat](img/cgroup_memory_stat.jpg)

![cgroup_memory_stat_cn](./img/cgroup_memory_stat_cn.jpg)

### 压力通知机制

当cgroup内的内存使用量达到某种压力状态的时候，内核可以通过eventfd的机制来通知用户程序，这个通知是通过cgroup.event_control和memory.pressure_level来实现的。使用步骤如下：

1. 使用eventfd()创建一个eventfd，假设叫做efd。
2. 然后open()打开memory.pressure_level的文件路径，产生一个fd，暂且称为pfd。
3. 然后将这两个fd和我们要关注的内存压力级别告诉内核，让内核帮我们关注条件是否成立。
4. 按这样的格式”<event_fd> <pressure_level_fd> <threshold>"写入cgroup.event_control。然后就等着efd是否可读。如果能读出信息，则代表内存使用已经触发了相关压力条件。

压力级别的level有三个：

1. low，表示内存使用已经达到触发内存回收的压力级别。
2. medium，表示内存使用压力更大了，已经开始触发swap以及将活跃的cache写回文件等操作了。
3. critical，意味内存已经达到上限，内核已经触发oom killer了。

从efd读取的消息内容就是这三个级别的关键字。**多个level可能要创建多个event_fd**。

### 内存阈值通知

使用cgroup的事件通知机制来对内存阈值进行监控，当内存使用量穿过（高于或低于）设置的阈值时，就会收到通知，步骤如下。

1. 使用eventfd()创建一个eventfd，假设叫做efd。
2. 然后open()打开memory.usage_in_bytes，产生一个fd，暂且称为ufd。
3. 往cgroup.event_control中写入这么一串：`<event_fd> <usage_in_bytes_fd> <threshold>`。
4. 然后通过读取event_fd得到通知。

## blkio资源限制

子系统为了减少进程之间共同读写同一块磁盘时相互干扰的问题。blkio子系统可以限制进程读写的IOPS和吞吐量，但它只能对Direct I/O的文件读写进行限速，对Buffered I/O的文件读写无法限制。

### 文件详情

1. blkio.io_service_bytes，被分组迁入或者移出磁盘的字节数量。它按操作类型（读或写，同步或异步）细分。头两个域定义了主次设备号，第三个域定义了操作类型，第四个域定义了字节数量。
2. blkio.io_serviced，被分组发给磁盘的IO(bio)数量。它按操作类型（读或写，同步或异步）细分。头两个域定义了主次设备号，第三个域定义了操作类型，第四个域定义了IO数量。
3. blkio.io_merged，cgroup内的bio请求总量。它按操作类型（读或写，同步或异步）细分。
4. blkio.io_queued，cgroup内任意给定时刻的排队请求总量。它按操作类型（读或写，同步或异步）细分。
5. blkio.throttle.io_service_bytes，被分组迁入或者移出磁盘的字节数量。它又按操作类型（读或写，同步或异步）细分。头两个域定义了主次设备号，第三个域定义了操作类型，第四个域定义了字节数量。
6. blkio.throttle.io_serviced，被分组迁入或者移出磁盘的字节数量。它又按操作类型（读或写，同步或异步）细分。头两个域定义了主次设备号，第三个域定义了操作类型，第四个域定义了字节数量。

## V1的缺陷

cgroup v1是每个层级对应一个子系统，子系统需要mount挂载使用，每个子系统之间是独立的，很难协同，比如memory subsys和blkio subsys能分别控制某个进程的资源使用，但blkio subsys对进程资源限制的时候无法感知memory subsys中进程资源的使用量。导致Buffer I/O的限制一直没有实现。

<img src="./img/cgroup-v1.jpg" alt="cgroup-v2" style="zoom:67%;" />

<img src="./img/cgroup-v1.jpg" alt="cgroup-v1" style="zoom: 67%;" />

# CGroupV2

## 资料

- [Linux Cgroup系列（05）：限制cgroup的CPU使用（subsystem之cpu） - SegmentFault 思否](https://segmentfault.com/a/1190000008323952?utm_source=sf-similar-article)
- [容器CPU（1）：怎么限制容器的CPU使用？_富士康质检员张全蛋的博客-CSDN博客_容器cpu限制](https://blog.csdn.net/qq_34556414/article/details/120654931)
- [3.2. cpu Red Hat Enterprise Linux 6 | Red Hat Customer Portal](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/resource_management_guide/sec-cpu)
- [Index of /doc/Documentation/cgroup-v1/ (kernel.org)](https://www.kernel.org/doc/Documentation/cgroup-v1/)
- [详解Cgroup V2 | Zorro’s Linux Book (zorrozou.github.io)](https://zorrozou.github.io/docs/详解Cgroup V2.html)
- [Linux内存中的Cache真的能被回收么？ | Zorro’s Linux Book (zorrozou.github.io)](https://zorrozou.github.io/docs/books/linuxnei-cun-zhong-de-cache-zhen-de-neng-bei-hui-shou-yao-ff1f.html)
- [Cgroup - Linux内存资源管理 | Zorro’s Linux Book (zorrozou.github.io)](https://zorrozou.github.io/docs/books/cgroup_linux_memory_control_group.html)
- [Memory Resource Controller — The Linux Kernel documentation](https://docs.kernel.org/admin-guide/cgroup-v1/memory.html)
- [A.4. cpuset Red Hat Enterprise Linux 7 | Red Hat Customer Portal](https://access.redhat.com/documentation/zh-cn/red_hat_enterprise_linux/7/html/resource_management_guide/sec-cpuset)
- [Linux 系统调用 eventfd - Notes about linux and my work (laoqinren.net)](http://linux.laoqinren.net/linux/syscall-eventfd/)
- [Simple command-line tool use cgroup's memory.pressure_level (github.com)](https://gist.github.com/vi/46f921db3cc24430f8d4)
- [Linux错误码汇总 - CodeAntenna](https://codeantenna.com/a/VRNs4eUMHL)
- [blkio cgroup · 田飞雨 (tianfeiyu.com)](https://blog.tianfeiyu.com/source-code-reading-notes/cgroup/blkio_cgroup.html)
- [Linux Cgroup v1(中文翻译)(4)：Block IO Controller - 啊噗得网 (apude.com)](https://www.apude.com/blog/14886.html)
- [Cgroup内核文档翻译(2)——Documentation/cgroup-v1/blkio-controller.txt - Hello-World3 - 博客园 (cnblogs.com)](https://www.cnblogs.com/hellokitty2/p/14226290.html)
- [[译\] Control Group v2（cgroupv2 权威指南）（KernelDoc, 2021） (arthurchiao.art)](https://arthurchiao.art/blog/cgroupv2-zh/)



