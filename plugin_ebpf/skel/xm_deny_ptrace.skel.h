/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */

/* THIS FILE IS AUTOGENERATED! */
#ifndef __XM_DENY_PTRACE_BPF_SKEL_H__
#define __XM_DENY_PTRACE_BPF_SKEL_H__

#include <stdlib.h>
#include <bpf/libbpf.h>

struct xm_deny_ptrace_bpf {
	struct bpf_object_skeleton *skeleton;
	struct bpf_object *obj;
	struct {
		struct bpf_map *xm_dp_evt_rb;
		struct bpf_map *rodata;
	} maps;
	struct {
		struct bpf_program *xm_bpf_deny_ptrace;
	} progs;
	struct {
		struct bpf_link *xm_bpf_deny_ptrace;
	} links;
	struct xm_deny_ptrace_bpf__rodata {
		__s64 target_pid;
	} *rodata;
};

static void
xm_deny_ptrace_bpf__destroy(struct xm_deny_ptrace_bpf *obj)
{
	if (!obj)
		return;
	if (obj->skeleton)
		bpf_object__destroy_skeleton(obj->skeleton);
	free(obj);
}

static inline int
xm_deny_ptrace_bpf__create_skeleton(struct xm_deny_ptrace_bpf *obj);

static inline struct xm_deny_ptrace_bpf *
xm_deny_ptrace_bpf__open_opts(const struct bpf_object_open_opts *opts)
{
	struct xm_deny_ptrace_bpf *obj;

	obj = (struct xm_deny_ptrace_bpf *)calloc(1, sizeof(*obj));
	if (!obj)
		return NULL;
	if (xm_deny_ptrace_bpf__create_skeleton(obj))
		goto err;
	if (bpf_object__open_skeleton(obj->skeleton, opts))
		goto err;

	return obj;
err:
	xm_deny_ptrace_bpf__destroy(obj);
	return NULL;
}

static inline struct xm_deny_ptrace_bpf *
xm_deny_ptrace_bpf__open(void)
{
	return xm_deny_ptrace_bpf__open_opts(NULL);
}

static inline int
xm_deny_ptrace_bpf__load(struct xm_deny_ptrace_bpf *obj)
{
	return bpf_object__load_skeleton(obj->skeleton);
}

static inline struct xm_deny_ptrace_bpf *
xm_deny_ptrace_bpf__open_and_load(void)
{
	struct xm_deny_ptrace_bpf *obj;

	obj = xm_deny_ptrace_bpf__open();
	if (!obj)
		return NULL;
	if (xm_deny_ptrace_bpf__load(obj)) {
		xm_deny_ptrace_bpf__destroy(obj);
		return NULL;
	}
	return obj;
}

static inline int
xm_deny_ptrace_bpf__attach(struct xm_deny_ptrace_bpf *obj)
{
	return bpf_object__attach_skeleton(obj->skeleton);
}

static inline void
xm_deny_ptrace_bpf__detach(struct xm_deny_ptrace_bpf *obj)
{
	return bpf_object__detach_skeleton(obj->skeleton);
}

static inline int
xm_deny_ptrace_bpf__create_skeleton(struct xm_deny_ptrace_bpf *obj)
{
	struct bpf_object_skeleton *s;

	s = (struct bpf_object_skeleton *)calloc(1, sizeof(*s));
	if (!s)
		return -1;
	obj->skeleton = s;

	s->sz = sizeof(*s);
	s->name = "xm_deny_ptrace_bpf";
	s->obj = &obj->obj;

	/* maps */
	s->map_cnt = 2;
	s->map_skel_sz = sizeof(*s->maps);
	s->maps = (struct bpf_map_skeleton *)calloc(s->map_cnt, s->map_skel_sz);
	if (!s->maps)
		goto err;

	s->maps[0].name = "xm_dp_evt_rb";
	s->maps[0].map = &obj->maps.xm_dp_evt_rb;

	s->maps[1].name = "xm_deny_.rodata";
	s->maps[1].map = &obj->maps.rodata;
	s->maps[1].mmaped = (void **)&obj->rodata;

	/* programs */
	s->prog_cnt = 1;
	s->prog_skel_sz = sizeof(*s->progs);
	s->progs = (struct bpf_prog_skeleton *)calloc(s->prog_cnt, s->prog_skel_sz);
	if (!s->progs)
		goto err;

	s->progs[0].name = "xm_bpf_deny_ptrace";
	s->progs[0].prog = &obj->progs.xm_bpf_deny_ptrace;
	s->progs[0].link = &obj->links.xm_bpf_deny_ptrace;

	s->data_sz = 2328;
	s->data = (void *)"\
\x7f\x45\x4c\x46\x02\x01\x01\0\0\0\0\0\0\0\0\0\x01\0\xf7\0\x01\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\xd8\x05\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\x40\0\x0d\0\
\x0c\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x47\x50\x4c\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x9f\xeb\x01\0\x18\0\0\0\0\0\0\0\x08\x02\0\0\
\x08\x02\0\0\x6b\x01\0\0\0\0\0\0\0\0\0\x02\x03\0\0\0\x01\0\0\0\0\0\0\x01\x04\0\
\0\0\x20\0\0\x01\0\0\0\0\0\0\0\x03\0\0\0\0\x02\0\0\0\x04\0\0\0\x1b\0\0\0\x05\0\
\0\0\0\0\0\x01\x04\0\0\0\x20\0\0\0\0\0\0\0\0\0\0\x02\x06\0\0\0\0\0\0\0\0\0\0\
\x03\0\0\0\0\x02\0\0\0\x04\0\0\0\0\0\x04\0\0\0\0\0\x02\0\0\x04\x10\0\0\0\x19\0\
\0\0\x01\0\0\0\0\0\0\0\x1e\0\0\0\x05\0\0\0\x40\0\0\0\x2a\0\0\0\0\0\0\x0e\x07\0\
\0\0\x01\0\0\0\0\0\0\0\0\0\0\x02\x0a\0\0\0\x37\0\0\0\x06\0\0\x04\x30\0\0\0\x52\
\0\0\0\x0b\0\0\0\0\0\0\0\x56\0\0\0\x0c\0\0\0\x40\0\0\0\x61\0\0\0\x0d\0\0\0\x80\
\0\0\0\x69\0\0\0\x0d\0\0\0\xc0\0\0\0\x6d\0\0\0\x0e\0\0\0\0\x01\0\0\x72\0\0\0\
\x0e\0\0\0\x40\x01\0\0\x77\0\0\0\0\0\0\x01\x08\0\0\0\x40\0\0\x01\x85\0\0\0\0\0\
\0\x08\x02\0\0\0\x8b\0\0\0\0\0\0\x08\x0b\0\0\0\x91\0\0\0\0\0\0\x08\x0f\0\0\0\
\x97\0\0\0\0\0\0\x01\x08\0\0\0\x40\0\0\0\0\0\0\0\x01\0\0\x0d\x0c\0\0\0\xae\0\0\
\0\x09\0\0\0\xb2\0\0\0\x01\0\0\x0c\x10\0\0\0\0\0\0\0\0\0\0\x0a\x13\0\0\0\0\0\0\
\0\0\0\0\x09\x0d\0\0\0\x3c\x01\0\0\0\0\0\x0e\x12\0\0\0\x01\0\0\0\x47\x01\0\0\0\
\0\0\x01\x01\0\0\0\x08\0\0\x01\0\0\0\0\0\0\0\x03\0\0\0\0\x15\0\0\0\x04\0\0\0\
\x04\0\0\0\x4c\x01\0\0\0\0\0\x0e\x16\0\0\0\x01\0\0\0\x55\x01\0\0\x01\0\0\x0f\0\
\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\x5b\x01\0\0\x01\0\0\x0f\0\0\0\0\x14\0\0\0\0\
\0\0\0\x08\0\0\0\x63\x01\0\0\x01\0\0\x0f\0\0\0\0\x17\0\0\0\0\0\0\0\x04\0\0\0\0\
\x69\x6e\x74\0\x5f\x5f\x41\x52\x52\x41\x59\x5f\x53\x49\x5a\x45\x5f\x54\x59\x50\
\x45\x5f\x5f\0\x74\x79\x70\x65\0\x6d\x61\x78\x5f\x65\x6e\x74\x72\x69\x65\x73\0\
\x78\x6d\x5f\x64\x70\x5f\x65\x76\x74\x5f\x72\x62\0\x73\x79\x73\x63\x61\x6c\x6c\
\x73\x5f\x65\x6e\x74\x65\x72\x5f\x70\x74\x72\x61\x63\x65\x5f\x61\x72\x67\x73\0\
\x70\x61\x64\0\x73\x79\x73\x63\x61\x6c\x6c\x5f\x6e\x72\0\x72\x65\x71\x75\x65\
\x73\x74\0\x70\x69\x64\0\x61\x64\x64\x72\0\x64\x61\x74\x61\0\x6c\x6f\x6e\x67\
\x20\x6c\x6f\x6e\x67\x20\x69\x6e\x74\0\x5f\x5f\x73\x33\x32\0\x5f\x5f\x73\x36\
\x34\0\x5f\x5f\x75\x36\x34\0\x6c\x6f\x6e\x67\x20\x6c\x6f\x6e\x67\x20\x75\x6e\
\x73\x69\x67\x6e\x65\x64\x20\x69\x6e\x74\0\x63\x74\x78\0\x78\x6d\x5f\x62\x70\
\x66\x5f\x64\x65\x6e\x79\x5f\x70\x74\x72\x61\x63\x65\0\x74\x72\x61\x63\x65\x70\
\x6f\x69\x6e\x74\x2f\x73\x79\x73\x63\x61\x6c\x6c\x73\x2f\x73\x79\x73\x5f\x65\
\x6e\x74\x65\x72\x5f\x70\x74\x72\x61\x63\x65\0\x2f\x68\x6f\x6d\x65\x2f\x70\x69\
\x6e\x67\x61\x6e\x2f\x50\x72\x6f\x67\x72\x61\x6d\x2f\x78\x2d\x6d\x6f\x6e\x69\
\x74\x6f\x72\x2f\x70\x6c\x75\x67\x69\x6e\x5f\x65\x62\x70\x66\x2f\x62\x70\x66\
\x2f\x78\x6d\x5f\x64\x65\x6e\x79\x5f\x70\x74\x72\x61\x63\x65\x2e\x62\x70\x66\
\x2e\x63\0\x20\x20\x20\x20\x72\x65\x74\x75\x72\x6e\x20\x30\x3b\0\x74\x61\x72\
\x67\x65\x74\x5f\x70\x69\x64\0\x63\x68\x61\x72\0\x5f\x6c\x69\x63\x65\x6e\x73\
\x65\0\x2e\x6d\x61\x70\x73\0\x2e\x72\x6f\x64\x61\x74\x61\0\x6c\x69\x63\x65\x6e\
\x73\x65\0\x9f\xeb\x01\0\x20\0\0\0\0\0\0\0\x14\0\0\0\x14\0\0\0\x1c\0\0\0\x30\0\
\0\0\0\0\0\0\x08\0\0\0\xc5\0\0\0\x01\0\0\0\0\0\0\0\x11\0\0\0\x10\0\0\0\xc5\0\0\
\0\x01\0\0\0\0\0\0\0\xea\0\0\0\x2e\x01\0\0\x05\xe0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\x28\0\0\0\x11\0\x04\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\x69\0\0\0\x11\
\0\x03\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\x31\0\0\0\x12\0\x02\0\0\0\0\0\0\0\0\
\0\x10\0\0\0\0\0\0\0\x74\0\0\0\x11\0\x05\0\0\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\
\xe8\x01\0\0\0\0\0\0\0\0\0\0\x05\0\0\0\0\x02\0\0\0\0\0\0\x0a\0\0\0\x03\0\0\0\
\x18\x02\0\0\0\0\0\0\0\0\0\0\x02\0\0\0\x2c\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\x40\
\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\x09\x07\x0a\0\x2e\x74\x65\x78\x74\0\x2e\x72\
\x65\x6c\x2e\x42\x54\x46\x2e\x65\x78\x74\0\x2e\x6d\x61\x70\x73\0\x2e\x6c\x6c\
\x76\x6d\x5f\x61\x64\x64\x72\x73\x69\x67\0\x5f\x6c\x69\x63\x65\x6e\x73\x65\0\
\x78\x6d\x5f\x62\x70\x66\x5f\x64\x65\x6e\x79\x5f\x70\x74\x72\x61\x63\x65\0\x74\
\x72\x61\x63\x65\x70\x6f\x69\x6e\x74\x2f\x73\x79\x73\x63\x61\x6c\x6c\x73\x2f\
\x73\x79\x73\x5f\x65\x6e\x74\x65\x72\x5f\x70\x74\x72\x61\x63\x65\0\x74\x61\x72\
\x67\x65\x74\x5f\x70\x69\x64\0\x78\x6d\x5f\x64\x70\x5f\x65\x76\x74\x5f\x72\x62\
\0\x2e\x73\x74\x72\x74\x61\x62\0\x2e\x73\x79\x6d\x74\x61\x62\0\x2e\x72\x6f\x64\
\x61\x74\x61\0\x2e\x72\x65\x6c\x2e\x42\x54\x46\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\x44\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\0\
\0\x10\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x91\0\0\
\0\x01\0\0\0\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x50\0\0\0\0\0\0\0\x08\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x29\0\0\0\x01\0\0\0\x03\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x58\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x14\0\0\0\x01\0\0\0\x03\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\x60\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x9d\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x70\
\0\0\0\0\0\0\0\x8b\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\x0b\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xfb\x03\0\0\0\0\0\0\
\x50\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x89\0\0\0\
\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x50\x04\0\0\0\0\0\0\x90\0\0\0\0\0\0\
\0\x0c\0\0\0\x02\0\0\0\x08\0\0\0\0\0\0\0\x18\0\0\0\0\0\0\0\x99\0\0\0\x09\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xe0\x04\0\0\0\0\0\0\x30\0\0\0\0\0\0\0\x08\0\0\
\0\x06\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x07\0\0\0\x09\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x10\x05\0\0\0\0\0\0\x20\0\0\0\0\0\0\0\x08\0\0\0\x07\0\0\
\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x1a\0\0\0\x03\x4c\xff\x6f\0\0\0\x80\0\0\
\0\0\0\0\0\0\0\0\0\0\x30\x05\0\0\0\0\0\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x81\0\0\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\x33\x05\0\0\0\0\0\0\xa2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0";

	return 0;
err:
	bpf_object__destroy_skeleton(s);
	return -1;
}

#endif /* __XM_DENY_PTRACE_BPF_SKEL_H__ */
