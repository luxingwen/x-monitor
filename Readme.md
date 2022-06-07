# x-monitor

系统指标采集服务。从传统指标、应用指标、ebpf内核观察三个层面来观察、收集指标。

1. [编译、运行](doc/编译、运行.md)

1. [系统监控](doc/系统指标采集.md)

1. [应用监控](doc/应用监控.md)

4. [指标查询](doc/指标查询.md)

5. #### 监控指标

   指标说明

   1. ##### cpu steal
      
      - 由于服务商在提供虚拟机时存在 CPU 超卖问题，因此和其他人共享 CPU 使用的情况是可能的。
      - 当发生 CPU 使用碰撞情况时，CPU 的使用取决于调度优先级；优先级低的进程会出现一点点 steal 值；若 steal 出现大幅升高，则说明由于超卖问题，把主机 CPU 占用满了。
      - 不使用虚拟机时一般不用关心该指标；使用虚拟机时，steal 代表超卖的幅度，一般不为 0 。

   2. ##### slab
      
      内核使用的所有内存片。
      
      - 可回收 reclaimable。
      - 不可回收 unreclaimable。

   3. ##### disk
      
      - storage inode util：小文件过多，导致 inode 耗光。
      - device/disk util：磁盘总 IO 量和理论上可行的 IO 量的比值，一般来说 util 越高，IO 时间越长。
      - 当 disk util 达到 100%，表示的不是 IO 性能低，而是 IO 需要排队，此时 CPU 使用看起来是下跌的，此时 cpu 的 iowait 会升高。
      - iostat中的重要指标
        - %util，磁盘I/O使用率。
        - r/s + w/s，就是IOPS
        - rkB/s + wkB/s，吞吐量
        - r_await + w_await，响应时间
      
   4. ##### memory
      
      - used = total - free - buffer - cache - slab reclaimable
      
      - util = used / total
        如果 util 操过 50%则认为是有问题的。若是 IO 密集型应用，在 util 操过 50%后一定要注意。

   5. ##### PSI(Pressure Stall Information)
      
      使用的 load average 有几个缺点
      
      - load average 的计算包含了 TASK_RUNNING 和 TASK_UNINTERRUPTIBLE 两种状态的进程，TASK_RUNNING 是进程处于运行，或等待分配 CPU 的准备运行状态，TASK_UNINTERRUPTIBLE 是进程处于不可中断的等待，一般是等待磁盘的输入输出。因此 load average 的飙高可能是因为 CPU 资源不够，让很多 TASK_RUNNING 状态的进程等待 CPU，也可能是由于磁盘 IO 资源紧张，造成很多进程因为等待 IO 而处于 TASK_UNINTERRUPTIBLE 状态。可以通过 load average 发现系统很忙，但是无法区分是因为争夺 CPU 还是 IO 引起的。
      - load average 最短的时间窗口是 1 分钟。
      - load average 报告的是活跃进程的原始数据，还需要知道可用 CPU 核数，这样 load average 的值才有意义。
      
      当 CPU、内存或 IO 设备争夺激烈的时候，系统会出现负载的延迟峰值、吞吐量下降，并可能触发内核的 `OOM Killer`。PSI 字面意思就是由于资源（CPU、内存和 IO）压力造成的任务执行停顿。**PSI** 量化了由于硬件资源紧张造成的任务执行中断，统计了系统中任务等待硬件资源的时间。我们可以用 **PSI** 作为指标，来衡量硬件资源的压力情况。停顿的时间越长，说明资源面临的压力越大。PSI 已经包含在 4.20 及以上版本内核中。https://xie.infoq.cn/article/931eee27dabb0de906869ba05。
      
      开启 psi：
      
      - 查看所有内核启动，grubby --info=ALL
      
      - 增加内核启动参数：grubby --update-kernel=/boot/vmlinuz-4.18.0 **--args=psi=1**，重启系统。
      
      - 查看 PSI 结果：
        
                tail /proc/pressure/*
                ==> /proc/pressure/cpu <==
                some avg10=0.00 avg60=0.55 avg300=0.27 total=1192936
                ==> /proc/pressure/io <==
                some avg10=0.00 avg60=0.13 avg300=0.06 total=325847
                full avg10=0.00 avg60=0.03 avg300=0.01 total=134192
                ==> /proc/pressure/memory <==
                some avg10=0.00 avg60=0.00 avg300=0.00 total=0
                full avg10=0.00 avg60=0.00 avg300=0.00 total=0

   6. ##### 网络
      
      1. 网卡
         /proc/net/dev：网卡指标文件 ，使用命令ip -s- s link
         /sys/class/net/：该目录下会有所有网卡的子目录，目录下包含了网口的配置信息，包括 deviceid，状态等。
         /sys/devices/virtual/net/：目录下都是虚拟网卡，通过该目录可以区分系统中哪些是虚拟网卡
         命令：ip -s -s link，查看所有设备的状态、统计信息
      
      2. 协议统计，/proc/net/netstat，/proc/net/snmp
         
         1. ECN。拥塞重传包，TCP 通过发送端和接收端以及**中间路由器**的配合，感知中间路径的拥塞，并主动减缓 TCP 的丢包决策。其实使用和路由器配合的方式，TCP 将网络路径中所有转发设备看做是黑盒，中间路由器如果过载丢包，发送端 TCP 是没法感知的，只有在定时器超时之后，而这个定时器相对较长，通常几秒到几十秒不等。TCP 现有的拥塞控制：慢启动、快输重传、快速恢复。
            
            +-----+-----+
            
            | ECN FIELD |
            
            +-----+-----+
            ECT CE [Obsolete] RFC 2481 names for the ECNbits.
            0 0 Not-ECT
            0 1 ECT(1)
            1 0 ECT(0)
            1 1 CE
         
         2. SYN Cookie。该技术对于超过 backlog 长度的 SYN 包使用 cookie 技术，可以让服务器收到客户端 SYN 报文时，不分配资源保存客户端信息，而是将这些信息保存在 SYN+ACK 的初始序号和时间戳中。对正常的连接，这些信息会随着 ACK 报文被带回来。该特性一般不会触发，只有 tcp_max_syn_backlog 队列占满时才会。
         
         3. Reorder。当发现了需要更新某条 TCP 流的 reordering 值（乱序值）时，会可能使用 4 种乱序计数器。
         
         4. TCP OFO（Out-Of-Order）。乱序的数据包被保存在 TCP 套接口的 out_of_order_queue 队列（实际是个红黑树）中。原因一般因为网络拥塞，导致顺序包抵达时间不同，延时太长，或者丢包，需要重新组合数据单元，因为数据包可能由不同的路径到达。
      
      3. 套接字，/proc/net/socksat
      
      4. 连接跟踪

6. #### 相关知识

   1. HZ，USER_HZ，Tick和jiffies。

      HZ：Linux核心每隔固定周期会发出timer interrupt (IRQ 0)，Hz是用来定义每一秒有几次timer interrupt，Hz可以在内核编译时设定，在我的系统上是1000。

      ```
      grep 'CONFIG_HZ=' /boot/config-$(uname -r)
      ```

      USER_HZ：通过getconf CLK_TCK来获取，可见该系统HZ是100。timers系统调用是用来统计进程时间消耗的，times系统调用的时间单位是由USER_HZ决定的。Linux通过/proc文件系统向用户空间提供了系统内部状态信息，/proc/stat中cpu节拍累加，它用的单位就是USER_HZ。

      ```
      [root@localhost ~]# getconf CLK_TCK
      100
      ```

      在程序中获取方式：

      ```c
      #include <unistd.h>
      #include <time.h>
      #include <stdio.h>
      
      int main()
      {
          struct timespec res;
          double resolution;
      
          printf("UserHZ   %ld\n", sysconf(_SC_CLK_TCK));
      
          clock_getres(CLOCK_REALTIME, &res);
          resolution = res.tv_sec + (((double)res.tv_nsec)/1.0e9);
      
          printf("SystemHZ %ld\n", (unsigned long)(1/resolution + 0.5));
          return 0;
       }
      ```

      Tick：是Hz的倒数，意思是timer interrupt每发生一次中断的时间，如果Hz是100，tick就是10毫秒。

      jiffies：是linux核心变量，它用来记录系统自开机以来已经经历过多少的Tick，每发生一次timer interrupt其都会加一。

      将jiffies转换为秒和毫秒：

      ```
      jiffies / HZ          /* jiffies to seconds */
      jiffies * 1000 / HZ   /* jiffies to milliseconds */
      ```

   2. 缺页错误：malloc 是扩展虚拟地址空间，应用程序使用 store/load 来使用分配的内存地址，这就用到虚拟地址到物理地址的转换。该虚拟地址没有实际对应的物理地址，这会导致 MMU 产生一个错误 page fault。

   3. RSS。进程所使用的全部物理内存数量称为常驻集大小（RSS），包括共享库等。

      RSS的计算，它不包括交换出去的内存（does not include memory that is swapped out），它包含共享库加载所使用的内存（It does include memory from shared libraries as long as the pages from those libraries are actually in memory），这个意思是共享库的代码段加载所使用的内存。它还包括stack和heap的内存。

      例如：一个进程它有500k的二进制文件，同时链接了2500k的共享库，分配了200k的stack/heap，但实际使用了100k的物理内存（其余的可能被swap或没有用），实际加载了1000k的共享库和400k自己的二进制，所以

      ```
      RSS: 400K + 1000K + 100K = 1500K
      VSZ: 500K + 2500K + 200K = 3200K
      ```

      **由于有些内存是共享的，许多进程都可以使用，所以将所有的RSS加起来会超过系统的内存大小**。

      进程的rss计算

      ```
      #define get_mm_rss(mm) (get_mm_counter(mm, file_rss) + get_mm_counter(mm, anon_rss))
      ```

      即RSS = file_rss + anon_rss。

      SHR=file_rss，进程使用的共享内存，也是算到file_rss的，因为共享内存基于tmpfs。

      ```
      unsigned long task_statm(struct mm_struct *mm,
                   unsigned long *shared, unsigned long *text,
                   unsigned long *data, unsigned long *resident)
      {
          *shared = get_mm_counter(mm, MM_FILEPAGES) +
                  get_mm_counter(mm, MM_SHMEMPAGES);
          *text = (PAGE_ALIGN(mm->end_code) - (mm->start_code & PAGE_MASK))
                                      >> PAGE_SHIFT;
          *data = mm->data_vm + mm->stack_vm;
          *resident = *shared + get_mm_counter(mm, MM_ANONPAGES);
          return mm->total_vm;
      }
      ```

      通过/proc/<pid>/smaps文件来查看进程的rss，rss的计算公式：**RSS = Private_Clean + Private_Dirty + Shared_Clean + Shared_Dirty**。

      **share/private**：该页面是共享还是私有。

      **dirty/clean**：该页面是否被修改过。

      以上4项的计算公式，查看该page引用计数，如果是1就是private，否则就是shared，同时查看page的flag，是否标记为_PAGE_DIRTY。**这其实是从page的角度来看进程的内存使用**。

   4. PSS (proportional set size)：实际使用的物理内存，共享库等按比例分配。如果上面1000k加载的共享库被两个进程使用，所以PSS的计算为：

      ```
      PSS：400K + （1000K/2) + 100k = 1000K
      ```

      在/proc/<pid>/smaps中，Pss实际包含private_clean + private_dirty，和按比例均分的shared_clean，shared_dirty。实际举个例子：进程A有x个private_clean页面，有y个private_dirty页面，有z个shared_clean仅和进程B共享，有h个shared_dirty页面和进程B、C共享。那么进程A的Pss为：
       `x + y + z/2 + h/3`。

   5. 通过进程实际对比RSS、PSS、USS

      这里使用工具procrank和文件/proc/<pid>/status。

      ```
      VmRSS:	        6328 kB
      RssAnon:	    2592 kB
      RssFile:	    3736 kB
      RssShmem:	       0 kB
      
        PID       Vss      Rss      Pss      Uss  cmdline
      377702    28176K    6328K    2838K    2592K  -bash
      ```

   6. USS：进程独占的物理内存，不计算共享库等的内存占用，在/proc/<pid>/smaps中就是private_clean + private_dirty。[What is RSS and VSZ in Linux memory management - Stack Overflow](https://stackoverflow.com/questions/7880784/what-is-rss-and-vsz-in-linux-memory-management)。

   7. Buffer和Cache的区别

      - 从两者的字面上解释，前者是缓冲区后者是缓存。man free手册可看到Buffer对应的是/proc/meminfo中的Buffers值，Cache是page cache和Slab用到的内存，对应/proc/meminfo中的Cached和SReclaimable


      - 可以近似认为是一样的东西。cache 对应块对象，底层是 block 结构，4k；buffer 对应文件对象，底层是 dfs 结构。可以粗略的认为 cache+buffer 是总的缓存。
    
        解释下Page Cache和Buffer Cache：The term, Buffer Cache, is often used for the Page Cache. Linux kernels up to version 2.2 had both a Page Cache as well as a Buffer Cache. As of the 2.4 kernel, these two caches have been combined. Today, there is only one cache, the Page Cache
    
        在命令free -m输出中，cached字段标识的就是page cache。
    
        - 当在写数据的时候，可见cache在递增，dirty page也在递增。直到数据写入磁盘，dirty page才会清空，但cache没有变化。
    
        ```
        [calmwu@192 Downloads]$ dd if=/dev/zero of=testfile.txt bs=1M count=100
        100+0 records in
        100+0 records out
        104857600 bytes (105 MB, 100 MiB) copied, 0.354432 s, 296 MB/s
        [calmwu@192 Downloads]$ free -m -w
                      total        used        free      shared     buffers       cache   available
        Mem:          15829         883       13994          18           3         948       14582
        Swap:          5119           0        5119
        [calmwu@192 Downloads]$ dd if=/dev/zero of=testfile1.txt bs=1M count=100
        100+0 records in
        100+0 records out
        104857600 bytes (105 MB, 100 MiB) copied, 0.040854 s, 2.6 GB/s
        [calmwu@192 Downloads]$ free -m -w
                      total        used        free      shared     buffers       cache   available
        Mem:          15829         883       13894          18           3        1048       14582
        Swap:          5119           0        5119
        [calmwu@192 Downloads]$ cat /proc/meminfo | grep Dirty
        Dirty:            102420 kB
        [calmwu@192 Downloads]$ sync
        [calmwu@192 Downloads]$ cat /proc/meminfo | grep Dirty
        Dirty:                 0 kB
        [calmwu@192 Downloads]$ free -m -w
                      total        used        free      shared     buffers       cache   available
        Mem:          15829         882       13893          18           3        1049       14583
        Swap:          5119           0        5119
        ```
    
        - Reading，读取的数据同样会缓存在page cache中，cache字段也会增大。
    
        **直白的说，Page Cache就是内核对磁盘文件内容在内存中的缓存**。

   8. SWAP。当系统内存需求超过一定水平时，内核中 kswapd 就开始寻找可以释放的内存。

      1. 文件系统页，从磁盘中读取并且没有修改过的页（backed by disk，磁盘有备份的页），例如：可执行代码、文件系统的元数据。
      2. 被修改过的文件系统页，就是 dirty page，这些页要先写回磁盘才可以被释放。
      3. 应用程序内存页，这些页被称为匿名页（anonymous memory），因为这些页不是来源于某个文件。如果系统中有换页设备（swap 分区），那么这些页可以先存入换页设备。
      4. 内存不够时，将页换页到换页设备上这一般会导致应用程序运行速度大幅下降。有些生产系统根本不配置换页设备。当没有换页设备时，系统出现内存不足情况，内核就会调用内存溢出进程终止程序杀掉某个进程。

   9. Out of socket memory。两种情况会发生

      1. 有很多孤儿套接字(orphan sockets)
      2. tcp 用尽了给他分配的内存。

      查看内核分配了多少内存给 TCP，这里的单位是 page，4096bytes

      ```
      [calmwu@192 build]$ cat /proc/sys/net/ipv4/tcp_mem
      187683    250244    375366
      ```

      当 tcp 使用的 page 少于 187683 时，kernel 不对其进行任何的干预
      当 tcp 使用了超过 250244 的 pages 时，kernel 会进入 “memory pressure”
      当 tcp 使用的 pages 超过 375366 时，我们就会看到题目中显示的信息
      查看 tcp 实际使用的内存，实际使用的 2，是远小于最低设置的。那么就只有可能是 orphan socket 导致的了。

      ```
      [calmwu@192 build]$ cat /proc/net/sockstat
      sockets: used 672
      TCP: inuse 9 orphan 0 tw 0 alloc 14 mem 2
      UDP: inuse 6 mem 1
      UDPLITE: inuse 0
      RAW: inuse 0
      FRAG: inuse 0 memory 0
      ```

   10. 进程内存使用和cgroup的内存统计的差异

      一般来说，业务进程使用的内存主要有以下几种情况：
    
      - 用户空间的匿名映射页，比如调用malloc分配的内存，以及使用MAP_ANONYMOUS的mmap；当系统内存不够时，内核可以将这部分内存交换出去。
      - 用户空间的文件映射（Mapped pages in User Mode address spaces），包含**map file**和**map tmpfs**，前者比如指定文件的mmap，后者比如**IPC共享内存**；当前内存不够时，内核可以回收这些页，但回收之前要先与文件同步数据。**共享库文件占用（代码段、数据段）的内存归属这类**。
      - 文件缓存，也称为**页缓存**（page in page cache of disk file），发生在文件read/write读写文件时，当系统内存不够时，内核可以回收这些页，但回收之前可能需要与文件同步数据。缓存的内容包括文件的内容，以及I/O缓冲的信息，该缓存的主要作用是提高文件性能和目录I/O性能。页缓存相比其他缓存来说尺寸是最大的，因为它不仅仅缓存文件的内容，还包括哪些被修改过但是还没有写回磁盘的页内容
      - buffer page，输入page cache，比如读取块设备文件。
    
      其中，1、2算作进程的RSS，3、4输入**page cache**。
    
      进程rss和cgroup rss的区别
    
      - 进程的rss = file_rss + filepage + shmmempage，cgroup_rss为每个cpu的vmstats_local->stat[NR_ANON_MAPPED]，其不包含共享内存，如果cgroup只包含匿名的，那么limit仅仅限制malloc分配的内存。
        
        ```
        static const unsigned int memcg1_stats[] = {
            NR_FILE_PAGES,
            NR_ANON_MAPPED,
        #ifdef CONFIG_TRANSPARENT_HUGEPAGE
            NR_ANON_THPS,
        #endif
            NR_SHMEM,
            NR_FILE_MAPPED,
            NR_FILE_DIRTY,
            NR_WRITEBACK,
            MEMCG_SWAP,
        };
        
        static const char *const memcg1_stat_names[] = {
            "cache",
            "rss",
        #ifdef CONFIG_TRANSPARENT_HUGEPAGE
            "rss_huge",
        #endif
            "shmem",
            "mapped_file",
            "dirty",
            "writeback",
            "swap",
        };
        
        memcg_page_state_local(memcg, memcg1_stats[i]);
        
        for_each_possible_cpu(cpu)
            x += per_cpu(memcg->vmstats_local->stat[idx], cpu);
        ```
        
        - cgroup cache包含file cache和共享内存。