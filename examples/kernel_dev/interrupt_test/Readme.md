# Linux dynamic debug

在文件/sys/kernel/debug/dynamic_debug/control中去开关pr_debug，dynamic_pr_debug，print_hex_dump_bytes等函数的输出。

下面是针对内核模块开启所有的debug输出。

```
 echo "module interrupt_test -p" > /sys/kernel/debug/dynamic_debug/control
 echo "module interrupt_test +p" > /sys/kernel/debug/dynamic_debug/control
```
