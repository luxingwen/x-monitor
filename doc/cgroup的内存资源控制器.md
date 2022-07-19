# cgroup memory controller

## 目的

- 隔离一个或一组应用
- 限制内存的使用量。

## 特性

- 统计匿名页、file cache、swap cache使用情况并加以限制。
- 统计memory+swap使用情况并加以限制。
- 使用量阈值通知。
- 内存压力通知。

## 设计

内存控制器的核心就是page_counter，它追踪添加到控制器里的进程当前内存使用情况以及使用限制，每个cgroup都有一个独立的内存控制器数据结构mem_cgroup。memcontrol.h

```
struct mem_cgroup {
	struct cgroup_subsys_state css;

	/* Private memcg ID. Used to ID objects that outlive the cgroup */
	struct mem_cgroup_id id;

	/* Accounted resources */
	struct page_counter memory;		/* Both v1 & v2 */
```

## 页交换扩展（CONFIG_MEMCG_SWAP）

```
[root@VM-0-8-centos /]# cat /boot/config-5.14.0-86.el9.x86_64|grep CONFIG_MEMCG_SWAP
CONFIG_MEMCG_SWAP=y
```

页交换扩展使得cgroup能记录交换的页。交换页被记录统计时，会增加如下文件

- memory.memsw.usage_in_bytes
- memory.memsw.limit_in_bytes

memsw是memory+swap的意思。cgroup限制进程所使用的内存总量实际是memsw，一般在服务器上，不会使用swap空间，文章[Linux交换空间 - Notes about linux and my work (laoqinren.net)](http://linux.laoqinren.net/linux/linux-swap/)介绍了该如何配置swap。

这个选项给memory resource controller添加了swap的管理功能，这样就可以针对每个cgroup限定其使用的mem+swap总量，如果关闭此选项，mrc将京能限制物理内存的使用量，而无法对swap进行控制，开启此功能会对性能有不利影响，并且追踪swap的使用也会消耗更多内存。

关闭方式：grubby --update-kernel=ALL --args=swapaccount=0，默认开启也可以通过内核引导参数"swapaccount=0"禁止此特性。**设置重启后memory.memsw.*文件就没有了**。

## 统计

### memory.usage_in_bytes

通用的解释是显示当前已用的内存，如果cgroup中只有一个进程，那么是该进程的内存使用量，但是和进程角度的内存使用是不同的（pidstat）。可以看到usage_in_bytes接近等于**rss + pagecache**，可以认为memory resource controller主要限制的是RSS和Page Cache。

```
 ⚡ root@localhost  /sys/fs/cgroup/memory/x-monitor  cat memory.usage_in_bytes 
53755904
 ⚡ root@localhost  /sys/fs/cgroup/memory/x-monitor  cat memory.stat 
cache 24195072
rss 28147712
```

### RSS的差异

用pidstat或top看进程的rss才115736，cgroup.memory.stat.rss和/proc/pid/stat.rss区别。

```
# Time        UID      TGID       TID    %usr %system  %guest   %wait    %CPU   CPU  minflt/s  majflt/s     VSZ     RSS   %MEM   kB_rd/s   kB_wr/s kB_ccwr/s iodelay   cswch/s nvcswch/s  Command
03:23:49 PM     0     10505         -    0.00    0.00    0.00    0.00    0.00     4    105.00      0.00 21474979248  115736   0.71      0.00     32.00      0.00       0      0.00      0.00  x-monitor
```

cgroup的memory stat文件内容解释

```
# per-memory cgroup local status
cache		- # of bytes of page cache memory.
rss		- # of bytes of anonymous and swap cache memory (includes
		transparent hugepages).
rss_huge	- # of bytes of anonymous transparent hugepages.
mapped_file	- # of bytes of mapped file (includes tmpfs/shmem)
pgpgin		- # of charging events to the memory cgroup. The charging
		event happens each time a page is accounted as either mapped
		anon page(RSS) or cache page(Page Cache) to the cgroup.
pgpgout		- # of uncharging events to the memory cgroup. The uncharging
		event happens each time a page is unaccounted from the cgroup.
swap		- # of bytes of swap usage
dirty		- # of bytes that are waiting to get written back to the disk.
writeback	- # of bytes of file/anon cache that are queued for syncing to
		disk.
inactive_anon	- # of bytes of anonymous and swap cache memory on inactive
		LRU list.
active_anon	- # of bytes of anonymous and swap cache memory on active
		LRU list.
inactive_file	- # of bytes of file-backed memory on inactive LRU list.
active_file	- # of bytes of file-backed memory on active LRU list.
unevictable	- # of bytes of memory that cannot be reclaimed (mlocked etc).
```

差异的说明：

- [Linux processes in memory and memory cgroup statistics - linux - newfreesoft.com](http://www.newfreesoft.com/linux/linux_processes_in_memory_and_memory_cgroup_statistics_747/)

### 判断

判断memory cgroup的真实内存使用量，不能看memory.usage_in_bytes，而需要用memory.stat.rss字段，这类似于free命令看到的，要看除去Page Cache之后的available字段。

