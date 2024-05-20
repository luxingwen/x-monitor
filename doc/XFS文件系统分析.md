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
trace-cmd start -p function -l 'xfs_alloc_*' -l 'xfs_bmap*' --func-stack
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
kprobe -s -H 'r:xfs_buf_get_map $retval'
```

##### 查看函数子调用

```
trace-cmd record -p function_graph -g xfs_file_fallocate
trace-cmd record -p function_graph -g xfs_trans_commit
trace-cmd record -p function_graph -g __xfs_log_force_lsn
trace-cmd record -p function_graph -g xlog_cil_push_work 
trace-cmd record -p function_graph  -P 265793 -P 264849 --max-graph-depth 15
trace-cmd record -p function_graph -g xfs_log_commit_cil  --max-graph-depth 2 
trace-cmd record -p function_graph -g xfs_buf_read_map
trace-cmd record -p function_graph -g xfs_bmap_btalloc
```

record之后使用trace-cmd report -L -I -S > report.txt来查看子调用过程

##### 跟踪xfs的tracepoint

```
perf trace -a -e 'xfs_log_force*' --kernel-syscall-graph --comm --failure --call-graph dwarf --max-event 4
perf trace -a -e 'xfs:xfs_log_force*' --kernel-syscall-graph --comm --failure --call-graph dwarf --max-event 4
perf trace -a -e 'xfs:xlog_cil_push_work' --kernel-syscall-graph --comm --failure --call-graph dwarf 
perf trace -e 'xfs:xfs_ail_move' -a --kernel-syscall-graph --comm --failure --call-graph dwarf/fp
perf trace -e 'xfs:xfs_ail_mo*-a --kernel-syscall-graph --comm --failure --call-graph fp
perf trace -e 'xfs:xfs_ail_*' -a --kernel-syscall-graph --comm --failure --call-graph fp
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

### 代码逻辑

#### xfs_trans_commit

`xfs_trans_commit` 是 XFS 文件系统中用于提交事务的函数。它负责将事务中的所有修改记录到日志中，并确保这些修改最终应用到文件系统中。

1. **锁定**：锁定事务和相关资源，以防止并发修改。
2. **更新元数据**：将事务中的修改应用到内存中的元数据结构。
3. **日志记录**：将事务的修改记录到 XFS 日志中。如果启用了 CIL（Committed Item List），则将修改记录到 CIL 中。
4. **释放资源**：释放事务占用的锁和资源。
5. **提交**：确保日志记录被推送到磁盘。
CIL 是一个用于优化日志记录性能的结构。`xfs_trans_commit` 将事务的修改记录到 CIL 中，而不是直接写入磁盘日志。CIL 会定期将批量的修改推送到磁盘，从而减少日志写入的开销。
`xfs_trans_commit` 本身不会直接将日志写入磁盘，而是将修改记录到 CIL 中。实际的日志写入由 CIL 的后台推送机制处理。

#### xlog_cil_push_background

`xlog_cil_push_background` 函数用于在后台将 CIL 中的记录推送到磁盘日志中，触发工作队列函数**xlog_cil_push_work**

1. **检查推送条件**：检查是否需要进行推送，例如 CIL 是否已满或达到推送间隔时间。
2. **锁定 CIL**：防止其他事务修改 CIL。
3. **创建日志记录**：将 CIL 中的修改打包成一个日志记录。
4. **写入磁盘**：将日志记录写入磁盘。
5. **更新状态**：更新 CIL 的状态和相关元数据。

#### xfs_cil_prepare_item

xfs_cil_prepare_item会给事务的所有xfs_log_item设置lsn号

```
	/*
	 * If this is the first time the item is being committed to the
	 * CIL, store the sequence number on the log item so we can
	 * tell in future commits whether this is the first checkpoint
	 * the item is being committed into.
	 */
	if (!lv->lv_item->li_seq)
		lv->lv_item->li_seq = log->l_cilp->xc_ctx->sequence;
}
```

#### 不同的tail_lsn、lsn

```
struct xfs_log_item {
....
	xfs_lsn_t			li_lsn;		/* last on-disk lsn */
	xfs_lsn_t			li_seq;		/* CIL commit seq */
};
```

```
typedef struct xlog_rec_header {
	__be32	  h_magicno;	/* log record (LR) identifier		:  4 */
	......
	__be64	  h_lsn;	/* lsn of this LR			:  8 */
	__be64	  h_tail_lsn;	/* lsn of 1st LR w/ buffers not committed: 8 */
} xlog_rec_header_t;
```

**`h_lsn` ≠ `li_seq`**：`h_lsn` 是日志记录的 LSN，而 `li_seq` 是日志项在 CIL 中的提交序列号。这两个字段的含义和用途不同，不应相等。

**`h_tail_lsn` ≠ `li_lsn`**：`h_tail_lsn` 是最早未提交日志记录的 LSN，而 `li_lsn` 是日志项最后一次写入的 LSN。这两个字段也不应相等。

**`agf_lsn` 与 `li_lsn`**: 在 XFS 中，当一个日志项（如 AGF 结构体的更新）被写入日志文件时，`li_lsn` 字段会被更新为该日志项最后一次写入磁盘的 LSN。因此，`agf_lsn` 通常会等于 `li_lsn`，因为它记录了 AGF 结构体的最后写入序列号。

#### 给xfs_in core log设置tail_lsn

遍历 AIL 链表，找到最小的 LSN

```
xfs_lsn_t xlog_assign_tail_lsn_locked(struct xfs_mount *mp)
{
	......
	lip = xfs_ail_min(mp->m_ail);
	if (lip)
		tail_lsn = lip->li_lsn;
	else
		tail_lsn = atomic64_read(&log->l_last_sync_lsn);
```

设置当前状态是：想要同步这个iclog；不再写入

```
static bool __xlog_state_release_iclog(struct xlog *log,
				       struct xlog_in_core *iclog)
{
	.......
	if (iclog->ic_state == XLOG_STATE_WANT_SYNC) {
		/* update tail before writing to iclog */
		xfs_lsn_t tail_lsn = xlog_assign_tail_lsn(log->l_mp);
		// 开始写入 iclog
		iclog->ic_state = XLOG_STATE_SYNCING;
		iclog->ic_header.h_tail_lsn = cpu_to_be64(tail_lsn);
		xlog_verify_tail_lsn(log, iclog, tail_lsn);
		/* cycle incremented when incrementing curr_block */
		return true;
	}

	ASSERT(iclog->ic_state == XLOG_STATE_ACTIVE);
	return false;
}
```

#### xlog_cil_insert_format_items

`xlog_cil_insert_format_items` 函数用于将格式化的日志项插入到 CIL 中。

1. **获取 CIL 上下文**：获取当前事务的 CIL 上下文。
2. **格式化日志项**：将事务中的修改项格式化为日志项。
3. **插入 CIL**：将格式化的日志项插入到 CIL。
4. **更新状态**：更新 CIL 的状态，例如增加已用空间。

#### 各个格式化函数的功能逻辑

这些函数用于将不同类型的日志项格式化，以便记录到日志或 CIL 中。

`xfs_bud_item_format`

用于格式化 "buffer update done"（BUD）日志项。这些项记录缓冲区更新已完成。

`xfs_buf_item_format`

用于格式化缓冲区日志项。这些项记录缓冲区的修改。

`xfs_qm_dquot_logitem_format`

用于格式化配额（quota）日志项。这些项记录配额管理相关的修改。

`xfs_qm_qoff_logitem_format`

用于格式化配额关闭（quota off）日志项。这些项记录配额系统关闭的操作。

`xfs_icreate_item_format`

用于格式化 inode 创建日志项。这些项记录新的 inode 创建操作。

`xfs_inode_item_format`

用于格式化 inode 日志项。这些项记录 inode 的修改。

#### xlog_cil_push_work

`xlog_cil_push_work` 是一个内核函数，用于将 CIL（Committed Item List）中的日志项推送到磁盘日志中。

1. **获取 CIL 上下文**：获取当前的 CIL 上下文。
2. **创建新的日志记录**：将 CIL 中的所有脏的日志项打包成一个新的日志记录。
3. **格式化日志**：格式化这些日志项以便写入磁盘。
4. **写入磁盘**：调用 `submit_bio` 或类似的函数将日志记录写入磁盘。
5. **更新状态**：更新 CIL 和事务的状态。
6. **插入 AIL 链表**：将已提交的日志项插入到 AIL（Active Item List）中，以便后续处理。

#### xfs_log持久化的状态

```
/*
 * In core log state
 */
enum xlog_iclog_state {
	XLOG_STATE_ACTIVE, /* Current IC log being written to */
	XLOG_STATE_WANT_SYNC, /* Want to sync this iclog; no more writes 当 core log 准备好要同步到磁盘时，它会进入此状态。这表示不再向日志写入新的条目，并且日志准备好要写入磁盘。*/
	XLOG_STATE_SYNCING, /* This IC log is syncing 当 core log 正在被写入磁盘时，它会处于此状态。这表示同步过程正在进行中，日志条目正在从内存缓冲区传输到磁盘存储*/
	XLOG_STATE_DONE_SYNC, /* Done syncing to disk 当 core log 成功同步到磁盘后，它会进入此状态。这表示所有日志条目已写入磁盘，并且日志已准备好可以再次使用*/
	XLOG_STATE_CALLBACK, /* Callback functions now 当 core log 处于此状态时，它表示正在执行与日志相关的回调函数。这些回调函数可能包括通知其他组件日志已同步或执行其他与日志相关的操作*/
	XLOG_STATE_DIRTY, /* Dirty IC log, not ready for ACTIVE status 当 core log 包含未同步的条目时，它会处于此状态。这表示日志尚未准备好可以再次写入，需要先同步到磁盘*/
	XLOG_STATE_IOERROR, /* IO error happened in sync'ing log 如果在同步 core log 时遇到 I/O 错误，它会进入此状态。这表示同步过程由于 I/O 错误而失败，可能需要采取进一步的操作来处理错误并恢复日志的一致性*/
};
```

#### xlog_write

xfs_log持久化，也就是将事务的记录的操作日志写入磁盘，核心函数是**xlog_write_copy_finish**，**xlog_write_iclog**

#### xlog_state_set_callback

cli的in core log写入磁盘后后触发ail

```
static void xlog_state_set_callback(struct xlog *log,
				    struct xlog_in_core *iclog,
				    xfs_lsn_t header_lsn)
{
	// 状态从 XLOG_STATE_DONE_SYNC 切换到 XLOG_STATE_CALLBACK
	iclog->ic_state = XLOG_STATE_CALLBACK;

	.......
	// cil in core log 写入磁盘后，就插入 ail 列表，同时调用 wake_up_process 唤醒 xfsalid 内核线程
	xlog_grant_push_ail(log, 0);
}
```

#### xlog_state_done_syncing

xfs in core log写入磁盘完成，将iclog的状态改为同步完成，同时开始进行callback，同时会唤醒在ic_write_wait等待的task，让其继续运行

```
STATIC void xlog_state_done_syncing(struct xlog_in_core *iclog)
{
	struct xlog *log = iclog->ic_log;

	/*
	 * If we got an error, either on the first buffer, or in the case of
	 * split log writes, on the second, we shut down the file system and
	 * no iclogs should ever be attempted to be written to disk again.
	 */
	if (!XLOG_FORCED_SHUTDOWN(log)) {
		// xfs 没有被强制 shutdonw，且状态是 XLOG_STATE_SYNCING，修改状态为 XLOG_STATE_DONE_SYNC
		ASSERT(iclog->ic_state == XLOG_STATE_SYNCING);
		iclog->ic_state = XLOG_STATE_DONE_SYNC;
	}
	// 唤醒一个等待的task
	wake_up_all(&iclog->ic_write_wait);
	.......
	xlog_state_do_callback(log);
}
```

可以在这里加入一个kprobe，用来输出iclog->ic_header的h_lsn和h_tail_lsn，

#### xlog_ioend_work

这是一个工作队列函数，一个bio完成，回调该函数通知写入完成，通知xfs in core log写入磁盘完成

#### xfs_alloc_vextent

##### 功能

`xfs_alloc_vextent` 函数用于分配一个或多个虚拟扩展（extent），这是 XFS 文件系统中块分配的核心函数之一。该函数根据指定的分配参数（如目标 AG、所需块数等）尝试分配实际的磁盘块。

##### 逻辑

1. **初始化分配环境**：
   - 初始化分配请求的参数和环境，包括目标 AG、所需块数、分配策略等。
2. **搜索空闲块**：
   - 在指定的 AG 中搜索满足条件的空闲块。
3. **执行块分配**：
   - 根据搜索结果执行块分配操作，并更新相关的元数据结构。
4. **返回结果**：
   - 返回分配结果，包括分配的块号和状态。

#### xfs_alloc_fix_freelist

#### struct xfs_buf

**`xfs_buf`** 是 XFS 文件系统中的缓冲区结构，用于管理和缓存特定的磁盘块（block），包括元数据块和数据块。`xfs_buf` 结构与文件系统的低级 I/O 操作紧密结合。

当文件系统执行读操作时，Page Cache 会首先检查所请求的数据是否在缓存中。如果在缓存中，则直接返回数据。

如果数据不在 Page Cache 中，则文件系统会从磁盘读取数据。这时，`xfs_buf` 可能被用来读取整个块，并将数据加载到 Page Cache 中。

当文件系统执行写操作时，数据首先写入 Page Cache。内核会在适当的时候将 Page Cache 中的数据刷新到磁盘。

XFS 文件系统使用 `xfs_buf` 来管理这些写入操作，确保数据完整性和一致性。

#### struct xfs_perag 与 struct xfs_agf 的关系

`xfs_perag` 和 `xfs_agf` 都与 XFS 文件系统的分配组（AG，Allocation Group）相关，但它们的作用和内容不同。

`xfs_perag` 结构用于在内存中管理和维护每个分配组的元数据。它包含了分配组的运行时信息，如空闲块计数、引用计数等。是内存运行时的结构。

`xfs_agf` （Allocation Group Free）是磁盘上的结构，包含分配组的空闲块信息。`xfs_agf` 结构存储在每个分配组的 AGF 块中，并记录该分配组中的空闲空间信息。是磁盘上持久化结构，当文件系统挂载时，xfs_agf的内容会被读取并加载到xfs_perag中。xfs_agf的信息会存放在xfs_buf中，

资料

[内核基础设施——wait queue - Notes about linux and my work (laoqinren.net)](https://linux.laoqinren.net/kernel/wait-queue/)

[workqueue（linux kernel 工作队列） - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/469007029)

