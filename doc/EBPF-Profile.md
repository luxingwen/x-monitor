# Profile

## 搭建环境

1. 安装podman

   ```
   dnf -y install podman-docker
   ```

2. 创建网络namespace

   ```
   docker network create xm-pyroscope 
   ```

3. 安装grafana

   ```
   docker pull grafana/grafana:main
   docker run --rm --name=grafana -d -p 3000:3000 -e "GF_FEATURE_TOGGLES_ENABLE=flameGraph" --network=xm-pyroscope  grafana/grafana:main
   ```

4. 安装pyroscope

   ```
   docker pull grafana/pyroscope:1.0.0
   docker run --rm --name pyroscope -d --network=xm-pyroscope -p 4040:4040 --network=xm-pyroscope grafana/pyroscope:1.0.0
   ```

   二进制启动

5. 配置

   - 查看容器ip

     ```
     docker exec -it 7738b5e2ee19 ip addr
     ```

     

## 安装x-monitor.eBPF

1. 配置
2. 启动

## 测试

1. 配置grafana-agent-flow

   ```
   logging {
   	level = "debug"
   }
   
   pyroscope.ebpf "instance" {
   	forward_to = [pyroscope.write.endpoint.receiver]
   	default_target = {
   		"__name__"="host_cpu",
   		"service_name"="host_cpu", 
   	}
           targets = [
   		{"__container_id__"="4ec0624267de013f5b936c7bc37a6551d95a997fa659d41a9f840e7d32b84378", 
   		 "service_name"="grafana", 
   		 "__name__"="grafana_cpu"},
   	]
   	targets_only = false
   	collect_kernel_profile = true
   	collect_user_profile = true
   	collect_interval = "5s"
   	sample_rate = 100
   }
   
   pyroscope.write "endpoint" {
   	endpoint {
           url = "http://10.0.0.4:4040"
       }
   }
   
   ```

2. 启动grafana-agent-flow

   ```
   ./grafana-agent-flow run /home/pingan/Program/agent/build/grafana-agent-flow.river
   ```

3. profile

   ![profile](img/profile.png)

## 栈帧回溯

### frame pointer(fp)

1. 需要一个ebp寄存器来保存frame pointer，每次调用都会将ebp赋值给esp。
2. 保存ebp寄存器即保存回溯信息(unwind info)的动作会被编译成代码，有指令开销。
3. 在回溯堆栈时，除了恢复sp，不知道怎么恢复其他的寄存器。(例如gdb中的 frame n, info reg)。
4. 没有源语言信息。

这种栈帧结构，在x86_64中被抛弃。gcc 在64位编译器中默认不使用rbp保存栈帧地址，即不再保存上一个栈帧地址，因此在x86_64中也就无法使用这种栈回溯方式（不过可以使用**-fno-omit-frame-pointer**选项保存栈帧rbp）。

```
-fno-omit-frame-pointer
    push $rbp # 在进入下一个函数(callee)时，会保存调用函数（caller）的栈帧
    mov $rbp, $rsp # 进入下一个函数(callee)后，会将新的帧地址（rsp）赋值给rbp
```

![在这里插入图片描述](img/frame_point)

```c
int top(void) {
    for(;;) { }
}

int c1(void) {
    top();
}

int b1(void) {
    c1();
}

int a1(void) {
    b1();
}

int main(void) {
  a1();
}
```

![image-20230923213252557](img/sample_with_frame_pointers.png)

#### 地址计算

```
(gdb) p td_io_queue
$1 = {enum fio_q_status (struct thread_data *, struct io_u *)} 0x55555557dbc0 <td_io_queue>

➜  utils git:(master) ✗ cat /proc/684299/maps
555555554000-55555560c000 r-xp 00000000 08:02 3314393                    /usr/bin/fio
55555580c000-55555580e000 r--p 000b8000 08:02 3314393                    /usr/bin/fio
55555580e000-5555558ee000 rw-p 000ba000 08:02 3314393                    /usr/bin/fio

55555557dbc0 - 555555554000 = 29BC0

➜  bin git:(feature-xm-ebpf-collector) ✗ readelf -s /usr/bin/fio|grep td_io_queue
   611: 0000000000029bc0  1692 FUNC    GLOBAL DEFAULT   15 td_io_queue
```

#### DWARF

调试信息（Debugging with Attributed Record Formats）定义了一个.debug_frame section，该调试信息支持处理无基地址的方法，可以将ebp用作常规寄存器，但是当保存esp时，它必须在.debug_frame节中产生一个注释，告诉调试器什么指令将其保存在何处。

不足在于

1. 没有源语言信息
2. 不支持在程序加载是同时加载调试信息

#### EH_FRAME

LSB(Linux Standard Base)标准中定义了一个.eh_frame section来解决上面的问题。这个section和.debug_frame非常类似，但是它编码紧凑，可以随程序一起加载。

这个段中存储着跟函数入栈相关的关键数据。当函数执行入栈指令后，在该段会保存跟入栈指令一一对应的编码数据，根据这些编码数据，就能计算出当前函数栈大小和cpu的那些寄存器入栈了，在栈中什么位置。

无论是否有-g选项，gcc都会默认生成.eh_frame和eh_frame_hdr这两个section。

1. 拥有源语言信息
2. 编码紧凑，并随程序一起加载。

#### DWARF’s Call Frame Information

每个.eh_frame section 包含一个或多个**CFI**(Call Frame Information)记录，记录的条目数量由.eh_frame 段大小决定。每条CFI记录包含一个**CIE**(Common Information Entry Record)记录，每个CIE包含一个或者多个**FDE**(Frame Description Entry)记录。

通常情况下，**CIE对应一个文件，FDE对应一个函数**。

**CFA (Canonical Frame Address, which is the address of %rsp in the caller frame)，CFA就是上一级调用者的堆栈指针**。

### eh_frame进行unwind

![img](img/eh_frame_unwind)

这张表的结构设计如下：

1. 第一列。表示程序中包含代码的每个位置的地址。（在共享对象中，这是一个相对于对象的偏移量。）其余的列包含与指示的位置关联的虚拟展开规则。

利用eh_frame来进行栈回朔的过程：

1. 根据当前的PC在.eh_frame中找到对应的条目，根据条目提供的各种偏移计算其他信息。（这里能否通过ebpf来获取rip寄存器）
2. 首先根据CFA = rsp+4，把当前rsp+4得到CFA的值。再根据CFA的值计算出通用寄存器和返回地址在堆栈中的位置。
3. 通用寄存器栈位置计算。例如：rbx = CFA-56。
4. 返回地址ra的栈位置计算。ra = CFA-8。
5. 根据ra的值，重复步骤1到4，就形成了完整的栈回溯。

## EBPF获取堆栈

Linus [is not a great lover of DWARF](https://lkml.org/lkml/2012/2/10/356), so there is not and probably will not be in-kernel DWARF support. This is why [`bpf_get_stackid()`](https://github.com/torvalds/linux/blob/0d18c12b288a177906e31fecfab58ca2243ffc02/include/uapi/linux/bpf.h#L2064) and [`bpf_get_stack()`](https://github.com/torvalds/linux/blob/0d18c12b288a177906e31fecfab58ca2243ffc02/include/uapi/linux/bpf.h#L2932) will often return gibberish if frame pointers are not built into the userspace application. 

BFP 提供了 bpf_get_stackid()/bpf_get_stack() help func 来获取 userspace stack，但是它依赖于 userspace program 编译时开启了 frame pointer 的支持。

![image-20230924110034316](img/ebpf-with_fp)

## 资料

1. [Get started with Pyroscope | Grafana Pyroscope documentation](https://grafana.com/docs/pyroscope/latest/get-started/)

1. [基于ebpf的parca-agent profiling方案探究_ebpf_jupiter_InfoQ写作社区](https://xie.infoq.cn/article/739629c2c64a16d99cf370f00)

1. **[Unwind 栈回溯详解 - pwl999 - 博客园 (cnblogs.com)](https://www.cnblogs.com/pwl999/p/15534946.html)**

1. [linux 栈回溯(x86_64 ) - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/302726082)

1. **[DWARF-based Stack Walking Using eBPF (polarsignals.com)](https://www.polarsignals.com/blog/posts/2022/11/29/dwarf-based-stack-walking-using-ebpf/)**

1. [linux 栈回溯(x86_64 ) - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/302726082)

1. [Function Stack Unwinding · Opsnull](https://blog.opsnull.com/posts/function-stack-unwinding/)

1. [Developing eBPF profilers for polyglot cloud-native applications (lpc.events)](https://lpc.events/event/16/contributions/1361/attachments/1043/2004/Developing eBPF profilers for polyglot cloud-native applications (1).pdf)

1. [ELF文件格式 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/286088470)

   

