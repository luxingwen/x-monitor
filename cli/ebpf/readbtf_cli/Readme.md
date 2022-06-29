# 读取BTF信息

类似于bpftool btf dump命令。只要内核编译时加上了CONFIG_DEBUG_INFO_BTF=y，vmlinux就带上自描述信息。可以通过btf的api来读取。

## 编译

```
make readbtf_cli VERBOSE=1
```

## 从vmlinux中读取结构信息

```
./readbtf_cli -s fb_monspecs
```

输出

```
level:{2} libbpf: loading kernel BTF '/sys/kernel/btf/vmlinux': 0
[INFO] Module: '(null)' Symbol: fb_monspecs have BTF id:13271
typedef unsigned int __u32;

struct fb_chroma {
	__u32 redx;
	__u32 greenx;
	__u32 bluex;
	__u32 whitex;
	__u32 redy;
	__u32 greeny;
	__u32 bluey;
	__u32 whitey;
};

typedef unsigned char __u8;

typedef short unsigned int __u16;

struct fb_videomode;

struct fb_monspecs {
	struct fb_chroma chroma;
	struct fb_videomode *modedb;
	__u8 manufacturer[4];
	__u8 monitor[14];
	__u8 serial_no[14];
	__u8 ascii[14];
	__u32 modedb_len;
	__u32 model;
	__u32 serial;
	__u32 year;
	__u32 week;
	__u32 hfmin;
	__u32 hfmax;
	__u32 dclkmin;
	__u32 dclkmax;
	__u16 input;
	__u16 dpms;
	__u16 signal;
	__u16 vfmin;
	__u16 vfmax;
	__u16 gamma;
	__u16 gtf: 1;
	__u16 misc;
	__u8 version;
	__u8 revision;
	__u8 max_x;
	__u8 max_y;
};
```

## 资料

- [BTF (BPF Type Format): A Practical Guide (containiq.com)](https://www.containiq.com/post/btf-bpf-type-format)
- [c - call printf using va_list - Stack Overflow](https://stackoverflow.com/questions/5977326/call-printf-using-va-list)
- [BTF deduplication and Linux kernel BTF (nakryiko.com)](https://nakryiko.com/posts/btf-dedup/)

