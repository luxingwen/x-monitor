# XFS文件系统分析

### 核心流程

#### Commit Transaction

#### Transaction Persist

#### cil to ail

#### ali persistence

### XFS命令使用

#### xfs_info

#### xfs_metadump

#### xfs_db

#### xfs_logprint

#### xxd

### XFS延迟处理的内核技术

#### work_queue

#### delayed_work

#### wait-queue

当编写内核代码时，一些进程应该为一些事件等待或者睡眠，在LInux内核中有几种处理睡眠和唤醒的方法，waitqueue是处理这种情况的方法之一。每当一个进程必须等待一个事件（数据到达或进程终止）时，它应该进入睡眠状态，休眠导致进程挂起，释放处理器以供其他用途。一段时间后，进程将被唤醒，并在我们等待的时间到达时继续其工作。

waitqueue：等待事件的进程列表。用于等待某人在某个条件为真时唤醒进程，必须小心它们，**以确保没有条件没有竞争条件**（难道是一对一），使用waitqueue的三个步骤

##### 初始化waitqueue

include头文件<linux/wait.h>，初始化有分为静态方式

```
DECLARE_WAIT_QUEUE_HEAD(wq);
```

动态方式

```
wait_queue_head_t wq;
init_waitqueue_head(&wq);
```

##### 排队（Queuing）

一旦等待队列被声明和初始化，进程就可以使用它进入睡眠状态，有如下几个宏用于不同的用途，每一个宏都会将该任务添加到我们创建的waitqueue中，然后它将等待事件。

| 函数                             | 调用&参数                                                    | 说明                                                         |
| -------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| wait_event                       | wait_event(wq, condition);                                   | **`wq`** – the waitqueue to wait on,  **`condition`** – a C expression for the event to wait for。进程将进入睡眠状态（TASK_UNINTERRUPTIBLE）直到condition的值为true。**每次唤醒等待队列wq时**，都会检查contidition。 |
| wait_event_timeout               | **`timeout`** – timeout, in jiffies                          | 休眠（TASK_UNINTERRUPTIBLE），直到condition为true或者超时。<br />返回值：timeout之后，condition为false，返回0；timeout之后，condition为true，返回1；timeout之前，condition为true，返回剩余的jiffies。 |
| wait_event_cmd                   | **`wait_event_cmd(wq, condition, cmd1, cmd2);`**             | 休眠（TASK_UNINTERRUPTIBLE）,cmd1在睡眠之前执行，cmd2将在睡眠之后执行 |
| wait_event_interruptible         | **`wait_event_interruptible(wq, condition);`**               | 进入睡眠状态（TASK_INTERRUPTIBLE），直到condition为true或收到信号。每次唤醒等待队列wq时，都会检查condition。<br />返回值：如果被信号中断，函数将返回-ERESTARTSYS，如果condition评估为true，则返回0。 |
| wait_event_interruptible_timeout | **`wait_event_interruptible_timeout(wq, condition, timeout);`** | 等待事件中断超时，进入睡眠状态（TASK_INTERRUPTIBLE），直到condition为true，或接收到信号或超时。<br />返回值：timeout之后，condition为false，返回0；timeout之后，condition为true，返回1；timeout之前，condition为true，返回剩余的jiffies；如果被信号中断，返回-ERESTARTSYS。 |
| wait_event_killable              | **`wait_event_killable(wq, condition);`**                    | 休眠（ASK_KILLABLE）。返回值：如果被信号中断，返回-ERESTARTSYS，如果condition为true，返回0。 |

##### 唤醒（Waking Up Queued Task）

当一些任务由于等待队列而处于睡眠模式，我们可以使用下面的函数来唤醒这些任务

###### wake_up

从等待队列中唤醒一个处于**不可中断**睡眠状态的进程。

**`wake_up(&wq);`**wq：要唤醒的等待队列

###### wake_up_all

唤醒等待队列中的所有进程。

**`wake_up_all(&wq);`** wq：要唤醒的等待队列

###### wake_up_interruptible

只从等待队列中唤醒一个处于可中断睡眠状态的进程

**`wake_up_interruptible(&wq);`**

###### wait_up_snyc and wake_up_interruptilbe_snyc

通常，一个 `wake_up` 调用会导致立即重新调度，这意味着其他进程可能在wake_up返回之前运行。“同步”变体使任何被唤醒的进程都可以运行，但不会重新调度CPU。这是用来避免在已知当前进程将进入睡眠状态时重新调度，从而强制重新调度。请注意，被唤醒的进程可以立即在不同的处理器上运行，因此这些函数不应该提供互斥。

###### wake_up_process

### 分析命令

##### 查看函数调用堆栈

```
trace-cmd start -p function -l xlog_bio_end_io --func-stack
trace-cmd start -p function -l xlog_write_iclog --func-stack
trace-cmd start -p function -l xfs_fs_sync_fs --func-stack
trace-cmd start -p function -l xfs_log_work_queue --func-stack
trace-cmd start -p function -l xfs_trans_commit --func-stack
trace-cmd start -p function -l 'xlog_cil_push*' --func-stack
trace-cmd start -p function -l xlog_cil_push_now --func-stack
trace-cmd start -p function -l xlog_cil_push_background --func-stack
```

start命令后接上trace-cmd show看输出结果，还可以使用perf-tools工具集里的kprobe

```
kprobe -s -H p:xfs_iflush_cluster
kprobe -s -H p:__xfs_log_force_lsn
kprobe -s -H p:xfs_rw_bdev
kprobe -s -H p:xlog_write_iclog
kprobe -s -H p:xlog_cil_push_work
kprobe -s -H p:xlog_sync
kprobe 'r:__xlog_state_release_iclog $retval'
kprobe 'p:xlog_state_release_iclog '
kprobe -s -H p:xfs_agf_write_verify
```

##### 查看函数子调用

```
trace-cmd record -p function_graph -g xfs_file_fallocate
trace-cmd record -p function_graph -g xfs_trans_commit
trace-cmd record -p function_graph -g __xfs_log_force_lsn
trace-cmd record -p function_graph -g xlog_cil_push_work 
trace-cmd record -p function_graph  -P 265793 -P 264849 --max-graph-depth 15
trace-cmd record -p function_graph -g xfs_log_commit_cil  --max-graph-depth 2 
```

record之后使用trace-cmd report -L -I -S > report.txt来查看子调用过程

##### 跟踪xfs的tracepoint

```
perf trace -a -e 'xfs_log_force*' --kernel-syscall-graph --comm --failure --call-graph dwarf --max-event 4
perf trace -a -e 'xfs:xfs_log_force*' --kernel-syscall-graph --comm --failure --call-graph dwarf --max-event 4
perf trace -a -e 'xfs:xlog_cil_push_work' --kernel-syscall-graph --comm --failure --call-graph dwarf 
perf trace -e 'xfs:xfs_ail_move' -a --kernel-syscall-graph --comm --failure --call-graph dwarf/fp
perf trace -e 'xfs:xfs_ail_mo*-a --kernel-syscall-graph --comm --failure --call-graph fp
trace -e 'xfs:xfs_ail_*' -a --kernel-syscall-graph --comm --failure --call-graph fp
```

##### 查看XFS某个函数的代码

```
perf probe -L xfs_log_commit_cil -m /usr/lib/debug/lib/modules/4.18.0-348.7.1.el8_5.x86_64+debug/kernel/fs/xfs/xfs.ko.debug
```

##### 使用SystemTap查看函数产生

```
stap -L 'kernel.function("vfs_read")'
stap -L 'kernel.function("vfs_write")'
stap -L 'module("xfs").function("xfs_agf_write_verify")'
stap -L 'module("xfs").function("xlog_cil_push_background")'
```

## 资料

[内核基础设施——wait queue - Notes about linux and my work (laoqinren.net)](https://linux.laoqinren.net/kernel/wait-queue/)

[workqueue（linux kernel 工作队列） - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/469007029)

