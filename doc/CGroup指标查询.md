# CGroup指标查询

## 限制CPU使用
### cpuacct控制器
>>>>>>> c8dcc9498a9bcab27cdfe14694f55a8600cf5704

自动生成控制组中任务对CPU资源使用情况的报告。

- cpu.shares

  shares用来设置CPU的相对值，必须大于等于2，最后除以权重综合，算出相对比例，按比例分配CPU时间。该指是针对所有的CPU。

  cgroup A设置为100, cgroup B设置为300，那么cgroupA中的任务运行25%的CPU时间。对于一个**四核CPU**的系统来说，cgroup A中的任务可以100%占有某一个CPU，这个比例是相对整体的一个值。

  shares值有如下特点：

  1. 如果A不忙，没有使用25%的时间，那么剩余的CPU时间将会被系统分配给B，即B的CPU使用率可以超过75%。
  2. 在闲的时候，shares基本上不起作用，只有在CPU忙的时候起作用。
  3. 由于shares是一个绝对值，需要和其它cgroup的值进行比较才能得到自己的相对限额，如果在一个部署很多容器的机器上，cgroup的数量是变化的，所以这个限额也是变化的。

- cpu.cfs_quota_us和cpu.cfs_period_us

  cpu调度策略有两种：

  - 完全公平调度（Completely Fair Scheduler，CFS），按限额和比例分配两种方式进行资源限制。
  - 实时调度（Real-TimeScheduler），针对实时任务按周期分配固定的运行时间。

  