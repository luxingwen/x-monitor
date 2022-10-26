# eBPF CPU观察

## CPU调度器

系统内核中的调度单元主要是thread，这些thread也叫task。其它类型的调度单元还包括中断处理程序：这些可能是软件运行过程中产生的软中断，例如网络收包，也可能是引荐发出的硬中断。

Thread运行的三种状态

- ON-PROC：指正在CPU上运行的线程
- RUNNABLE：指可运行，但正在运行队列中排队等待的线程。CFS调度现在已经是红黑树来维护状态了。
- SLEEP：指正在等待其它事件，包括不可中断的等待线程。

RunQueue中的线程是按照优先级排序的，这个值可以由系统内核和用户进程来设置，通过调节这个值可以调高重要任务的运行性能。

有两种方式可以让线程哟里CPU执行：

- 主动脱离：它发生在线程阻塞于I/O、锁或者主动sleep。
- 被动脱离：如果线程运行时长超过调度器分配给其的CPU时间，或者高优先级抢占低优先级任务时，就会被调度器调离CPU。

**上下文切换**：当CPU从一个线程切换到另一个线程时，需要更换内存寻址信息和其它的上下文信息，这种行为就是“上下文切换”。

线程迁移：在多core环境中，有core空闲，并且运行队列中有可运行状态的线程等待执行，CPU调度器可以将这个线程迁移到该core的队列中，以便尽快运行。

### 调度策略

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

### 内核实现Tracepoint

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

### runqlat 

用来记录CPU调度器从唤醒一个线程到这个线程运行之间的时间间隔。

唤醒的tracepoint，使用的是btf raw tracepoint

- tp_btf/sched_wakeup，trace_sched_wakeup，入口函数是try_to_wake_up/ttwu_do_wakeup。
- tp_btf/sched_wakeup_new，实际函数是trace_sched_wakeup_new。入口函数是wake_up_new_task。

运行的tracepoint

- tp_btf/sched_switch，trace_sched_switch，入口函数__schedule，这因该是调度器入口。负责在运行队列中找到一个该运行的进程。到了这里进程就被cpu执行了。

#### 内核唤醒

唤醒操作是通过函数wake_up进行，它会唤醒指定的等待队列上的所有进程。它调用函数try_to_wake_up()，该函数负责将进程设置为TASK_RUNNING状态，调用enqueue_task()将此进程放入红黑树中，如果被唤醒的进程优先级比当前正在执行的进程优先级高，还要设置need_resched标志。

need_resched标志：来表明是否需要重新执行一次调度，该标志对于内核来讲是一个信息，**它表示有其他进程应当被运行了，要尽快调用调度程序**。相关代码在kernel/sched/core.c

## 资料

1. [【原创】Kernel调试追踪技术之 Ftrace on ARM64 - HPYU - 博客园 (cnblogs.com)](https://www.cnblogs.com/hpyu/p/14348523.html)
2. https://mozillazg.com/2022/06/ebpf-libbpf-btf-powered-enabled-raw-tracepoint-common-questions.html#hidsec
3. [Linux 内核静态追踪技术的实现 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/433010401)
4. [「Let's Go eBPF」认识数据源：Tracepoint | Serica (iserica.com)](https://www.iserica.com/posts/brief-intro-for-tracepoint/)

