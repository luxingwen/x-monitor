# CPU观察

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

### tracepoint函数

下面代码定义了一个tracepoint函数，DEFINE_EVENT宏会给sched_wakeup_new、sched_wakeup加上**前缀trace_**，实际内核代码会调用trace_sched_wakeup/trace_sched_wakeup_new。

```
/*
 * Tracepoint called when the task is actually woken; p->state == TASK_RUNNNG.
 * It is not always called from the waking context.
 */
DEFINE_EVENT(sched_wakeup_template, sched_wakeup,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint for waking up a new task:
 */
DEFINE_EVENT(sched_wakeup_template, sched_wakeup_new,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));
```

调用tracepoint函数trace_sched_wakup(p)。所以可以观察这些tracepoint函数，获得更准确的状态

```
static int
try_to_wake_up(struct task_struct *p, unsigned int state, int wake_flags)
{
	unsigned long flags;
	int cpu, success = 0;

	preempt_disable();
	if (p == current) {
		/*
		 * We're waking current, this means 'p->on_rq' and 'task_cpu(p)
		 * == smp_processor_id()'. Together this means we can special
		 * case the whole 'p->on_rq && ttwu_runnable()' case below
		 * without taking any locks.
		 *
		 * In particular:
		 *  - we rely on Program-Order guarantees for all the ordering,
		 *  - we're serialized against set_special_state() by virtue of
		 *    it disabling IRQs (this allows not taking ->pi_lock).
		 */
		if (!(p->state & state))
			goto out;

		success = 1;
		trace_sched_waking(p);
		p->state = TASK_RUNNING;
		trace_sched_wakeup(p);
		goto out;
	}
```

```
void wake_up_new_task(struct task_struct *p)
{
	struct rq_flags rf;
	struct rq *rq;

	raw_spin_lock_irqsave(&p->pi_lock, rf.flags);
	p->state = TASK_RUNNING;
#ifdef CONFIG_SMP
	/*
	 * Fork balancing, do it here and not earlier because:
	 *  - cpus_ptr can change in the fork path
	 *  - any previously selected CPU might disappear through hotplug
	 *
	 * Use __set_task_cpu() to avoid calling sched_class::migrate_task_rq,
	 * as we're not fully set-up yet.
	 */
	p->recent_used_cpu = task_cpu(p);
	rseq_migrate(p);
	__set_task_cpu(p, select_task_rq(p, task_cpu(p), WF_FORK));
#endif
	rq = __task_rq_lock(p, &rf);
	update_rq_clock(rq);
	post_init_entity_util_avg(p);

	activate_task(rq, p, ENQUEUE_NOCLOCK);
	trace_sched_wakeup_new(p);
	check_preempt_curr(rq, p, WF_FORK);
#ifdef CONFIG_SMP
	if (p->sched_class->task_woken) {
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

need_resched标志：来表明是否需要重新执行一次调度，该标志对于内核来讲是一个信息，它表示有其他进程应当被运行了，要尽快调用调度程序。相关代码在kernel/sched/core.c

## 资料

1. [【原创】Kernel调试追踪技术之 Ftrace on ARM64 - HPYU - 博客园 (cnblogs.com)](https://www.cnblogs.com/hpyu/p/14348523.html)
2. https://mozillazg.com/2022/06/ebpf-libbpf-btf-powered-enabled-raw-tracepoint-common-questions.html#hidsec

