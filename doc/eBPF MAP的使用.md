# eBPF MAP的使用

## BPF_MAP_TYPE_PERF_EVENT_ARRAY

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

