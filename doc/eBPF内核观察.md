# EBPF内核观察

eBPF源于BPF，本质上是处于内核中的一个高效与灵活的虚拟机组件，以一种安全的方式在许多内核hook点执行字节码。BPF最初的目的是用于搞笑网络保温过滤，经过重新设计，eBPF不再局限于网络协议栈，已经成为内核顶级的子系统，演进为一个通用的执行引擎。开发者可基于eBPF开发性能分析工具、软件定义网络，安全等诸多场景。

eBPF的网络应用：

- Packet Filtering
- Packet Forwarding
- Packet Tracing
- Network Scheduling

eBPF在存储的应用

- XRP，reduce the kernel software overhead for storage
  1. XRP is the first system to use eBPF to accelerate common storage functions
  2. XRP captures mos t of the performance benefit of kernel bypass, without sacrificing CPU utilization and access control.