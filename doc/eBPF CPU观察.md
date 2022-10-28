# eBPF CPU观察

## CPU调度器

系统内核中的调度单元主要是thread，这些thread也叫task。其它类型的调度单元还包括中断处理程序：这些可能是软件运行过程中产生的软中断，例如网络收包，也可能是引荐发出的硬中断。

Thread运行的三种状态

- ON-PROC：指正在CPU上运行的线程
- RUNNABLE：指可运行，但正在运行队列中排队等待的线程。CFS调度现在已经是红黑树来维护状态了。
- SLEEP：指正在等待其它事件，包括不可中断的等待线程。

RunQueue中的线程是按照优先级排序的，这个值可以由系统内核和用户进程来设置，通过调节这个值可以调高重要任务的运行性能。

有两种方式可以让线程脱离CPU执行：

- 主动脱离，任务主动通过直接或间接调用schedule函数引起的上下文切换。

- 被动脱离：如果线程运行时长超过调度器分配给其的CPU时间，或者高优先级抢占低优先级任务时，就会被调度器调离CPU。

**上下文切换**：当CPU从一个线程切换到另一个线程时，需要更换内存寻址信息和其它的上下文信息，这种行为就是“上下文切换”。一般操作系统因为以下三种方式引起上下文切换，

- Task Scheduling（进程上下文切换），任务调度一般是由调度器在内核空间完成，通常将当前CPU执行任务的代码，用户或内核栈，地址空间切换到下一个需要运行任务的代码，用户或内核栈，地址空间。这是一种整体切换。成本是比系统调用要高的。

  如果是同一个进程的线程间进行上下文切换，它们之间有一部分共用的数据，这些数据可以免除切换，所以开销较小。

- Interrupt or Exception

- System Call（特权模式切换），**同一进程内**的CPU上下文切换，相对于进程的切换应该少了保存用户空间资源这一步。

进程的上下文包括，虚拟内存，栈，全局变量，寄存器等用户空间资源，还包括内核堆栈、寄存器等内核空间状态。

线程迁移：在多core环境中，有core空闲，并且运行队列中有可运行状态的线程等待执行，CPU调度器可以将这个线程迁移到该core的队列中，以便尽快运行。

### 调度类

进程调度依赖调度策略，内核把相同的调度策略抽象成**调度类**。不同类型的进程采用不同的调度策略，Linux内核中默认实现了5个调度类

1. stop

2. deadline，用于调度有严格时间要求的实时进程，比如视频编解码

   ```
   const struct sched_class dl_sched_class = {
   	.next			= &rt_sched_class,
   	.enqueue_task		= enqueue_task_dl,
   	.dequeue_task		= dequeue_task_dl,
   	.yield_task		= yield_task_dl,
   	.check_preempt_curr	= check_preempt_curr_dl,
   ```

3. realtime

   ```
   const struct sched_class rt_sched_class = {
   	.next			= &fair_sched_class,
   	.enqueue_task		= enqueue_task_rt,
   	.dequeue_task		= dequeue_task_rt,
   	.yield_task		= yield_task_rt,
   	.check_preempt_curr	= check_preempt_curr_rt,
   ```

4. CFS，选择vruntime值最小的进程运行。nice越大，虚拟时间过的越慢。CFS使用红黑树来组织就绪队列，因此可以最快找到vruntime值最小的那个进程，pick_next_task_fair()

   ```
   const struct sched_class fair_sched_class = {
   	.next			= &idle_sched_class,
   	.enqueue_task		= enqueue_task_fair,
   	.dequeue_task		= dequeue_task_fair,
   	.yield_task		= yield_task_fair,
   	.yield_to_task		= yield_to_task_fair,
   ```

5. idle

调度类的结构体中关键元素

- enqueue_task：在运行队列中添加一个新进程
- dequeue_task：当进程从运行队列中移出时
- yield_task：当进程想自愿放弃CPU时
- pick_next_task：schedule()调用pick_next_task的函数。从它的类中挑选出下一个最佳可运行的任务。

组调度，CFS引入group scheduling，其中时间片被分配给线程组而不是单个线程。在组调度下，进程A及其创建的线程属于一个组，进程B及其创建的线程属于另一个组。

### 时间片

time slice是os用来表示进程被调度进来与被调度出去之间所能维持运行的时间长度。通常系统都有默认的时间片，但是很难确定多长的时间片是合适的。

### 调度入口

进程的调度是从调用通用调度器schedule函数开始的，schedule()功能是挑选下一个可运行任务，由pick_next_task()来遍历这些调度类选出下一个任务。该函数首先会在cfs类中寻找下一个最佳任务。

判断方式：运行队列中可运行的任务总数nr_running是否等于CFS类的子运行队列中的可运行任务的总数来判断。否则会遍历所有其它类并挑选下一个最佳可运行任务。

```
static inline struct task_struct *
pick_next_task(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	const struct sched_class *class;
	struct task_struct *p;

	/*
	 * Optimization: we know that if all tasks are in the fair class we can
	 * call that function directly, but only if the @prev task wasn't of a
	 * higher scheduling class, because otherwise those lose the
	 * opportunity to pull in more work from other CPUs.
	 */
	if (likely((prev->sched_class == &idle_sched_class ||
		    prev->sched_class == &fair_sched_class) &&
		   rq->nr_running == rq->cfs.h_nr_running)) {

		p = pick_next_task_fair(rq, prev, rf);
```

### Task状态

| TASK_RUNNING         | 等待运行状态                                                 |
| -------------------- | ------------------------------------------------------------ |
| TASK_INTERRUPTIBLE   | 可中断睡眠状态                                               |
| TASK_UNINTERRUPTIBLE | 不可中断睡眠状态                                             |
| TASK_STOPPED         | 停止状态（收到 SIGSTOP、SIGTTIN、SIGTSTP 、 SIG.TTOU信号后状态 |
| TASK_TRACED          | 被调试状态                                                   |
| TASK_KILLABLE        | 新可中断睡眠状态                                             |
| TASK_PARKED          | kthread_park使用的特殊状态                                   |
| TASK_NEW             | 创建任务临时状态                                             |
| TASK_DEAD            | 被唤醒状态                                                   |
| TASK_IDLE            | 任务空闲状态                                                 |
| TASK_WAKING          | 被唤醒状态                                                   |

## 内核Tracepoint的实现

下面代码定义了一个tracepoint函数，DEFINE_EVENT本质是封装了__DECLARE_TRACE宏，该宏定义了一个tracepoint结构对象。

```
struct tracepoint {
	const char *name;		/* Tracepoint name */
	struct static_key key;
	int (*regfunc)(void);
	void (*unregfunc)(void);
	struct tracepoint_func __rcu *funcs;
};

DEFINE_EVENT(sched_wakeup_template, sched_wakeup,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

#define DEFINE_EVENT(template, name, proto, args)		\
	DECLARE_TRACE(name, PARAMS(proto), PARAMS(args))
	
#define DECLARE_TRACE(name, proto, args)				\
	__DECLARE_TRACE(name, PARAMS(proto), PARAMS(args),		\
			cpu_online(raw_smp_processor_id()),		\
			PARAMS(void *__data, proto),			\
			PARAMS(__data, args))
			
#define __DECLARE_TRACE(name, proto, args, cond, data_proto, data_args) \
	extern struct tracepoint __tracepoint_##name;			\
	static inline void trace_##name(proto)				\
	{
```

内核函数会调用插桩点trace_sched_waking。

```
static int
try_to_wake_up(struct task_struct *p, unsigned int state, int wake_flags)
{
	unsigned long flags;
	int cpu, success = 0;

	preempt_disable();
	if (p == current) {
		if (!(p->state & state))
			goto out;

		success = 1;
		trace_sched_waking(p);
		p->state = TASK_RUNNING;
		trace_sched_wakeup(p);
		goto out;
	}
```

## BPF对CPU的分析能力

1. 为什么CPU系统调用时间很高？是系统调用导致的吗？具体是那些系统调用？
2. 线程每次唤醒时在CPU上花费了多长时间？
3. 线程在运行队列中等待的时间有多长？
4. 当前运行队列中有多少线程在等待执行？
5. 不同CPU之间的运行队列是否均衡？
6. 那些软中断和硬中断占用了CPU时间？
7. 当其它运行队列中有需要运行的程序时，那些CPU任然处于空闲状态。

### eBPF程序的消耗

跟踪CPU调度器事件，效率非常重要，因为上下文切换这样的调度器事件每秒可能触发几百万次。如果每次上下文切换都执行eBPF程序，累积起来性能消耗也是很可观的，10%的消耗是书上说的最糟糕。    

### runqlat 

Time a task spends waiting on a runqueue for it's turn to run on the processor，我第一感觉就是enqueue_task到schedule之间的时间消耗，**很像排队上机到上机的这段时间**。

#### 内核唤醒

唤醒操作是通过函数wake_up进行，它会唤醒指定的等待队列上的所有进程。它调用函数try_to_wake_up()，该函数负责将进程设置为TASK_RUNNING状态，调用enqueue_task()将此进程放入红黑树中，如果被唤醒的进程优先级比当前正在执行的进程优先级高，还要设置need_resched标志。

need_resched标志：来表明是否需要重新执行一次调度，该标志对于内核来讲是一个信息，**它表示有其他进程应当被运行了，要尽快调用调度程序**。相关代码在kernel/sched/core.c

我们通过工具来观察下try_to_wake_up函数设置TASK_RUNNING，并插入运行队列中的流程。

通过分析代码ttwu_do_activate函数中会调用**activate_task**和**ttwu_do_wakeup**。

```
static void ttwu_do_activate(struct rq *rq, struct task_struct *p,
			     int wake_flags, struct rq_flags *rf)
{
......
	activate_task(rq, p, en_flags);
	ttwu_do_wakeup(rq, p, wake_flags, rf);
}
```

函数active_task中会将task加入运行队列

```
void activate_task(struct rq *rq, struct task_struct *p, int flags)
{
	enqueue_task(rq, p, flags);

	p->on_rq = TASK_ON_RQ_QUEUED;
}
```

函数ttwu_do_wakeup会将任务状态设置为TASK_RUNNING，同时调用插桩的tracepoint

```
static void ttwu_do_wakeup(struct rq *rq, struct task_struct *p, int wake_flags,
			   struct rq_flags *rf)
{
	check_preempt_curr(rq, p, wake_flags);
	p->state = TASK_RUNNING;
	trace_sched_wakeup(p);
```

使用perf-tool的funcgraph来观察下ttwu_do_activate函数的子调用，看看是否会调用上面的两个函数。执行命令 **./funcgraph -d 1 -HtP -m 3 ttwu_do_activate > out** ， 的确，可以看到enqueue_task_fair和ttwu_do_wakeup的调用。funcgraph用来观察函数内的子调用，这点非常方便。

```
27050.671092 |   6)    <idle>-0    |               |  ttwu_do_activate() {
27050.671092 |   6)    <idle>-0    |               |    psi_task_change() {
27050.671092 |   6)    <idle>-0    |   0.078 us    |      record_times();
27050.671093 |   6)    <idle>-0    |   0.620 us    |    }
27050.671093 |   6)    <idle>-0    |               |    enqueue_task_fair() {
27050.671093 |   6)    <idle>-0    |   0.598 us    |      enqueue_entity();
27050.671094 |   6)    <idle>-0    |   0.027 us    |      hrtick_update();
27050.671094 |   6)    <idle>-0    |   1.106 us    |    }
27050.671094 |   6)    <idle>-0    |               |    ttwu_do_wakeup() {
27050.671094 |   6)    <idle>-0    |   0.188 us    |      check_preempt_curr();
```

从代码分析唤醒的调用堆栈：**try_to_wake_up()----->ttwu_queue()----->ttwu_do_activate()**。

使用bpftrace来观察下ttw_do_activate函数的调用堆栈，这是外部调用，bpftrace -e 'kprobe:ttwu_do_activate { @[kstack] = count(); }'，看到堆栈如下

```
@[
    ttwu_do_activate+1
    try_to_wake_up+422
    hrtimer_wakeup+30
    __hrtimer_run_queues+256
    hrtimer_interrupt+256
    smp_apic_timer_interrupt+106
    apic_timer_interrupt+15
    native_safe_halt+14
    acpi_idle_do_entry+70
    acpi_idle_enter+155
    cpuidle_enter_state+135
    cpuidle_enter+44
    do_idle+564
    cpu_startup_entry+111
    start_secondary+411
    secondary_startup_64_no_verify+194
]: 526
```

感觉使用bcc的trace工具看起来更直观，trace 'ttwu_do_activate' -K -T -a

```
15:27:02 0       0       swapper/0       ttwu_do_activate 
        ffffffffa711c141 ttwu_do_activate+0x1 [kernel]
        ffffffffa711d186 try_to_wake_up+0x1a6 [kernel]
        ffffffffa717d6ee hrtimer_wakeup+0x1e [kernel]
        ffffffffa717d920 __hrtimer_run_queues+0x100 [kernel]
        ffffffffa717e0f0 hrtimer_interrupt+0x100 [kernel]
        ffffffffa7a026ba smp_apic_timer_interrupt+0x6a [kernel]
        ffffffffa7a01c4f apic_timer_interrupt+0xf [kernel]
        ffffffffa79813ae native_safe_halt+0xe [kernel]
        ffffffffa7981756 acpi_idle_do_entry+0x46 [kernel]
        ffffffffa756a18b acpi_idle_enter+0x9b [kernel]
        ffffffffa77435a7 cpuidle_enter_state+0x87 [kernel]
        ffffffffa774393c cpuidle_enter+0x2c [kernel]
        ffffffffa7122a84 do_idle+0x234 [kernel]
        ffffffffa7122c7f cpu_startup_entry+0x6f [kernel]
        ffffffffa8c19267 start_kernel+0x51d [kernel]
        ffffffffa7000107 secondary_startup_64_no_verify+0xc2 [kernel]
```

#### 唤醒抢占（Wakeup Preemption）

当一个任务被唤醒，选择一个CPU的rq插入，然后将状态设置为TASK_RUNNING后，调度器会立即进行判断CPU当前运行的任务是否被抢占，一旦调度算法决定剥夺当前任务的运行，就会设置TIF_NEED_RESCHED标志。

继续跟踪ttwu_do_wakeup，core.c文件中函数check_preempt_curr逻辑中会调用resched_curr，这个函数会设置TIF_NEED_RESCHED标志，而调度类的实现函数sched_class->check_preempt_curr也会在流程判断是否调用resched_curr函数。

```
void check_preempt_curr(struct rq *rq, struct task_struct *p, int flags)
{
	const struct sched_class *class;

	if (p->sched_class == rq->curr->sched_class) {
		rq->curr->sched_class->check_preempt_curr(rq, p, flags);
	} else {
		for_each_class(class)
		{
			if (class == rq->curr->sched_class)
				break;
			if (class == p->sched_class) {
				resched_curr(rq);
				break;
			}
		}
	}
	.......
```

我们用bcc的trace来观察下resched_curr堆栈，**trace 'resched_curr' -K -T -a**。下面堆栈符合了check_preempt_curr函数逻辑

```
15:57:58 3050    3449    PLUGINSD[ebpf]  resched_curr     
        ffffffffa711ba31 resched_curr+0x1 [kernel]
        ffffffffa712970d check_preempt_wakeup+0x18d [kernel]
        ffffffffa711bfd2 check_preempt_curr+0x62 [kernel]
        ffffffffa711c019 ttwu_do_wakeup+0x19 [kernel]
        ffffffffa711da8c sched_ttwu_pending+0xcc [kernel]
        ffffffffa711dd84 scheduler_ipi+0xa4 [kernel]
        ffffffffa7a02425 smp_reschedule_interrupt+0x45 [kernel]
        ffffffffa7a01d2f reschedule_interrupt+0xf [kernel]
        
15:57:58 0       0       swapper/2       resched_curr     
        ffffffffa711ba31 resched_curr+0x1 [kernel]
        ffffffffa711bfea check_preempt_curr+0x7a [kernel]
        ffffffffa711c019 ttwu_do_wakeup+0x19 [kernel]
        ffffffffa711d186 try_to_wake_up+0x1a6 [kernel]
        ffffffffa71d4743 watchdog_timer_fn+0x53 [kernel]
        ffffffffa717d920 __hrtimer_run_queues+0x100 [kernel]
        ffffffffa717e0f0 hrtimer_interrupt+0x100 [kernel]
        ffffffffa7a026ba smp_apic_timer_interrupt+0x6a [kernel]
        ffffffffa7a01c4f apic_timer_interrupt+0xf [kernel]
        ffffffffa79813ae native_safe_halt+0xe [kernel]
        ffffffffa7981756 acpi_idle_do_entry+0x46 [kernel]
        ffffffffa756a18b acpi_idle_enter+0x9b [kernel]
        ffffffffa77435a7 cpuidle_enter_state+0x87 [kernel]
        ffffffffa774393c cpuidle_enter+0x2c [kernel]
        ffffffffa7122a84 do_idle+0x234 [kernel]
        ffffffffa7122c7f cpu_startup_entry+0x6f [kernel]
        ffffffffa705929b start_secondary+0x19b [kernel]
        ffffffffa7000107 secondary_startup_64_no_verify+0xc2 [kernel]
```

被抢占的进程被设置TIF_NEED_RESCHED后，并没有立即调用schedule函数发生上下文切换。

#### 唤醒的tracepoint

使用的是btf raw tracepoint

- tp_btf/sched_wakeup，trace_sched_wakeup，入口函数是try_to_wake_up/ttwu_do_wakeup。
- tp_btf/sched_wakeup_new，实际函数是trace_sched_wakeup_new。入口函数是wake_up_new_task。

#### 运行的tracepoint

- tp_btf/sched_switch，trace_sched_switch，入口函数__schedule，这因该是调度器入口。负责在运行队列中找到一个该运行的进程。到了这里进程就被cpu执行了。

## 资料

1. [【原创】Kernel调试追踪技术之 Ftrace on ARM64 - HPYU - 博客园 (cnblogs.com)](https://www.cnblogs.com/hpyu/p/14348523.html)
2. https://mozillazg.com/2022/06/ebpf-libbpf-btf-powered-enabled-raw-tracepoint-common-questions.html#hidsec
3. [Linux 内核静态追踪技术的实现 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/433010401)
4. [「Let's Go eBPF」认识数据源：Tracepoint | Serica (iserica.com)](https://www.iserica.com/posts/brief-intro-for-tracepoint/)
5. [linux 任务状态定义 – GarlicSpace](https://garlicspace.com/2019/06/29/linux任务状态定义/)
6. [CFS调度器（2）-源码解析 (wowotech.net)](http://www.wowotech.net/process_management/448.html)
7. [Linux Preemption - 2 | Oliver Yang](http://oliveryang.net/2016/03/linux-scheduler-2/)
8. [一文让你明白CPU上下文切换 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/52845869)

