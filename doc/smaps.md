# smaps文件

1. 操作初始化，task_mmu.c

   ```
   static const struct seq_operations proc_pid_smaps_op = {
   	.start	= m_start,
   	.next	= m_next,
   	.stop	= m_stop,
   	.show	= show_smap
   };
   ```

   

