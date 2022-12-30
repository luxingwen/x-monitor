# eBPF MAP的使用

## BPF_MAP_TYPE_PERF_EVENT_ARRAY

存储指向`perf_event`数据结构的指针，用于读取和存储perf事件计数器

构造一个map对象

```
struct {
	__uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
	__uint(key_size, sizeof(int));
	__uint(value_size, sizeof(u32));
	__uint(max_entries, 2);
} my_map SEC(".maps");
```

函数bpf_perf_event_output插入数据

```
	struct S {
		u64 pid;
		u64 cookie;
	} data;

	data.pid = bpf_get_current_pid_tgid();
	data.cookie = 0x12345678;

	bpf_perf_event_output(ctx, &my_map, 0, &data, sizeof(data));
```

## BPF_MAP_TYPE_TASK_STORAGE

在5.11内核版本中添加，为eBPF LSM提供基于task_struct的本地存储。为此添加了三个helpers：

1. bpf_task_storage_get
2. bpf_task_storage_delete
3. bpf_get_current_task_btf

构造一个map对象

```
struct {
	__uint(type, BPF_MAP_TYPE_TASK_STORAGE);
	__uint(map_flags, BPF_F_NO_PREALLOC);
	__type(key, int);
	__type(value, u64);
} start SEC(".maps");
```

为task创建一个本地存储，存放时间

```
	ptr = bpf_task_storage_get(&start, t, 0,
				   BPF_LOCAL_STORAGE_GET_F_CREATE);
	if (!ptr)
		return 0;
	*ptr = bpf_ktime_get_ns();
```

删除map数据

```
bpf_task_storage_delete(&start, next);
```

获取btf指针指向的task_struct结构

```
	curr = bpf_get_current_task_btf();
	pelem = bpf_task_storage_get(&tstamp, curr, NULL, 0);
```

## BPF_MAP_TYPE_HASH

一种哈希表，

## BPF_MAP_TYPE_STACK_TRACE

存储调用堆栈跟踪信息

## BPF_MAP_TYPE_PERCPU_ARRAY

## BPF_MAP_TYPE_RINGBUF

内核5.8引入，很多场景下需要将数据发送到user-space，例如log event。相对于之前的实际标准BPF_MAP_TYPE_PERF_EVENT_ARRAY解决了浪费内存、事件顺序无法保证的问题。ringbuf map有三个特点

- 降低内存开销
- 保证事件顺序
- 减少数据复制

ringbuf是一个多生产者，单消费者队列，可安全的在多个CPU之间共享和操作，在user-space可以使用epoll或busy loop两种方式获取数据。

在使用是，可以使用工具看看内核是否支持ringbuf map。

```
 ⚡ root@localhost  ~  bpftool feature
 eBPF map_type ringbuf is available
```

