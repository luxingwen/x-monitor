# CGroup指标查询

## 限制CPU资源
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

cfs_period_us：用来配置时间周期长度，单位：微秒。取值范围1毫秒(1000ms)~1秒(1000000ms)。它是CFS算法的一个调度周期，一般是100000。

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

总结这个场景

```
cpu.share and cpu.cfs_quota_us are working together.

Given a total cpu quota, we should firstly distribute the cpu.share of each cgroup. Then find the cgroups whose exact quota exceeds their cpu.cfs_quota_us, find all such cgroups and keep their quota as their cpu.cfs_quota_us, and collect the exceeded part as unused cpu pool. Distribute these unused cpu pool among other cgroups by cpu.share again, and iterate as above, until no cgroup is exceeding the upper limit.
```

#### cpu资源报告

提供了CPU资源用量的统计

- cpuacct.usage，统计控制组中所有任务的CPU使用时长，单位是ns
- cpuacct.stat，统计控制组中所有任务在用户态和内核态分别使用的CPU时长，单位是ns
- cpuacct.usage_percpu，统计控制组中所有任务使用每个CPU的时长，单位是ns
- cpu.stat
  - nr_periods：表示过去了多少个cpu.cfs_period_us里面配置的时间周期。
  - nr_throttled：在上面这些周期中，有多少次是受到了限制（即cgroup中的进程在指定的时间周期中用光了它的配额）。
  - throttled_time：cgroup中的进程被限制使用CPU持续了多长时间，单位是ns。

## 资料

- [Linux Cgroup系列（05）：限制cgroup的CPU使用（subsystem之cpu） - SegmentFault 思否](https://segmentfault.com/a/1190000008323952?utm_source=sf-similar-article)
- [容器CPU（1）：怎么限制容器的CPU使用？_富士康质检员张全蛋的博客-CSDN博客_容器cpu限制](https://blog.csdn.net/qq_34556414/article/details/120654931)
- [3.2. cpu Red Hat Enterprise Linux 6 | Red Hat Customer Portal](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/resource_management_guide/sec-cpu)



