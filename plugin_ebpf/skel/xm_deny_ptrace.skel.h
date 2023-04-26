/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */

/* THIS FILE IS AUTOGENERATED! */
#ifndef __XM_DENY_PTRACE_BPF_SKEL_H__
#define __XM_DENY_PTRACE_BPF_SKEL_H__

#include <errno.h>
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
	int err;

	obj = (struct xm_deny_ptrace_bpf *)calloc(1, sizeof(*obj));
	if (!obj) {
		errno = ENOMEM;
		return NULL;
	}

	err = xm_deny_ptrace_bpf__create_skeleton(obj);
	err = err ?: bpf_object__open_skeleton(obj->skeleton, opts);
	if (err)
		goto err_out;

	return obj;
err_out:
	xm_deny_ptrace_bpf__destroy(obj);
	errno = -err;
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
	int err;

	obj = xm_deny_ptrace_bpf__open();
	if (!obj)
		return NULL;
	err = xm_deny_ptrace_bpf__load(obj);
	if (err) {
		xm_deny_ptrace_bpf__destroy(obj);
		errno = -err;
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
		goto err;
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

	s->data_sz = 4208;
	s->data = (void *)"\
\x7f\x45\x4c\x46\x02\x01\x01\0\0\0\0\0\0\0\0\0\x01\0\xf7\0\x01\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\xf0\x0c\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\x40\0\x0e\0\
\x01\0\xbf\x17\0\0\0\0\0\0\x85\0\0\0\x0e\0\0\0\xbf\x06\0\0\0\0\0\0\x18\x01\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\x79\x12\0\0\0\0\0\0\x15\x02\x03\0\0\0\0\0\x79\x72\x18\
\0\0\0\0\0\x79\x13\0\0\0\0\0\0\x5d\x32\x1c\0\0\0\0\0\x77\x06\0\0\x20\0\0\0\x79\
\x14\0\0\0\0\0\0\x18\x01\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\xb7\x02\0\0\x34\0\0\0\
\xbf\x63\0\0\0\0\0\0\x85\0\0\0\x06\0\0\0\xb7\x01\0\0\x09\0\0\0\x85\0\0\0\x6d\0\
\0\0\xbf\x08\0\0\0\0\0\0\xb7\x09\0\0\0\0\0\0\x18\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\xb7\x02\0\0\x1c\0\0\0\xb7\x03\0\0\0\0\0\0\x85\0\0\0\x83\0\0\0\xbf\x07\0\0\0\
\0\0\0\x15\x07\x0a\0\0\0\0\0\x63\x87\x18\0\0\0\0\0\x63\x67\0\0\0\0\0\0\x63\x97\
\x04\0\0\0\0\0\xbf\x71\0\0\0\0\0\0\x07\x01\0\0\x08\0\0\0\xb7\x02\0\0\x10\0\0\0\
\x85\0\0\0\x10\0\0\0\xbf\x71\0\0\0\0\0\0\xb7\x02\0\0\0\0\0\0\x85\0\0\0\x84\0\0\
\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x78\x2d\x6d\x6f\x6e\x69\
\x74\x6f\x72\x20\x64\x65\x6e\x79\x20\x70\x69\x64\x3a\x27\x25\x6c\x64\x27\x20\
\x70\x74\x72\x61\x63\x65\x20\x74\x6f\x20\x74\x61\x72\x67\x65\x74\x20\x70\x69\
\x64\x3a\x27\x25\x6c\x64\x27\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x47\x50\
\x4c\0\x9f\xeb\x01\0\x18\0\0\0\0\0\0\0\x48\x02\0\0\x48\x02\0\0\xd0\x03\0\0\0\0\
\0\0\0\0\0\x02\x03\0\0\0\x01\0\0\0\0\0\0\x01\x04\0\0\0\x20\0\0\x01\0\0\0\0\0\0\
\0\x03\0\0\0\0\x02\0\0\0\x04\0\0\0\x1b\0\0\0\x05\0\0\0\0\0\0\x01\x04\0\0\0\x20\
\0\0\0\0\0\0\0\0\0\0\x02\x06\0\0\0\0\0\0\0\0\0\0\x03\0\0\0\0\x02\0\0\0\x04\0\0\
\0\0\0\x04\0\0\0\0\0\x02\0\0\x04\x10\0\0\0\x19\0\0\0\x01\0\0\0\0\0\0\0\x1e\0\0\
\0\x05\0\0\0\x40\0\0\0\x2a\0\0\0\0\0\0\x0e\x07\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\
\x02\x0a\0\0\0\x37\0\0\0\x06\0\0\x04\x30\0\0\0\x52\0\0\0\x0b\0\0\0\0\0\0\0\x56\
\0\0\0\x0c\0\0\0\x40\0\0\0\x61\0\0\0\x0d\0\0\0\x80\0\0\0\x69\0\0\0\x0d\0\0\0\
\xc0\0\0\0\x6d\0\0\0\x0e\0\0\0\0\x01\0\0\x72\0\0\0\x0e\0\0\0\x40\x01\0\0\x77\0\
\0\0\0\0\0\x01\x08\0\0\0\x40\0\0\x01\x81\0\0\0\0\0\0\x08\x02\0\0\0\x87\0\0\0\0\
\0\0\x08\x0b\0\0\0\x8d\0\0\0\0\0\0\x08\x0f\0\0\0\x93\0\0\0\0\0\0\x01\x08\0\0\0\
\x40\0\0\0\0\0\0\0\x01\0\0\x0d\x0c\0\0\0\xa6\0\0\0\x09\0\0\0\xaa\0\0\0\x01\0\0\
\x0c\x10\0\0\0\0\0\0\0\0\0\0\x0a\x13\0\0\0\0\0\0\0\0\0\0\x09\x0d\0\0\0\x86\x03\
\0\0\0\0\0\x0e\x12\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\x0a\x16\0\0\0\x91\x03\0\0\0\0\
\0\x01\x01\0\0\0\x08\0\0\x01\0\0\0\0\0\0\0\x03\0\0\0\0\x15\0\0\0\x04\0\0\0\x34\
\0\0\0\x96\x03\0\0\0\0\0\x0e\x17\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\0\0\0\x16\0\
\0\0\x04\0\0\0\x04\0\0\0\xb1\x03\0\0\0\0\0\x0e\x19\0\0\0\x01\0\0\0\xba\x03\0\0\
\x01\0\0\x0f\0\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\xc0\x03\0\0\x02\0\0\x0f\0\0\0\
\0\x14\0\0\0\0\0\0\0\x08\0\0\0\x18\0\0\0\x08\0\0\0\x34\0\0\0\xc8\x03\0\0\x01\0\
\0\x0f\0\0\0\0\x1a\0\0\0\0\0\0\0\x04\0\0\0\0\x69\x6e\x74\0\x5f\x5f\x41\x52\x52\
\x41\x59\x5f\x53\x49\x5a\x45\x5f\x54\x59\x50\x45\x5f\x5f\0\x74\x79\x70\x65\0\
\x6d\x61\x78\x5f\x65\x6e\x74\x72\x69\x65\x73\0\x78\x6d\x5f\x64\x70\x5f\x65\x76\
\x74\x5f\x72\x62\0\x73\x79\x73\x63\x61\x6c\x6c\x73\x5f\x65\x6e\x74\x65\x72\x5f\
\x70\x74\x72\x61\x63\x65\x5f\x61\x72\x67\x73\0\x70\x61\x64\0\x73\x79\x73\x63\
\x61\x6c\x6c\x5f\x6e\x72\0\x72\x65\x71\x75\x65\x73\x74\0\x70\x69\x64\0\x61\x64\
\x64\x72\0\x64\x61\x74\x61\0\x6c\x6f\x6e\x67\x20\x6c\x6f\x6e\x67\0\x5f\x5f\x73\
\x33\x32\0\x5f\x5f\x73\x36\x34\0\x5f\x5f\x75\x36\x34\0\x75\x6e\x73\x69\x67\x6e\
\x65\x64\x20\x6c\x6f\x6e\x67\x20\x6c\x6f\x6e\x67\0\x63\x74\x78\0\x78\x6d\x5f\
\x62\x70\x66\x5f\x64\x65\x6e\x79\x5f\x70\x74\x72\x61\x63\x65\0\x74\x72\x61\x63\
\x65\x70\x6f\x69\x6e\x74\x2f\x73\x79\x73\x63\x61\x6c\x6c\x73\x2f\x73\x79\x73\
\x5f\x65\x6e\x74\x65\x72\x5f\x70\x74\x72\x61\x63\x65\0\x2f\x68\x6f\x6d\x65\x2f\
\x63\x61\x6c\x6d\x77\x75\x2f\x50\x72\x6f\x67\x72\x61\x6d\x2f\x78\x2d\x6d\x6f\
\x6e\x69\x74\x6f\x72\x2f\x70\x6c\x75\x67\x69\x6e\x5f\x65\x62\x70\x66\x2f\x62\
\x70\x66\x2f\x78\x6d\x5f\x64\x65\x6e\x79\x5f\x70\x74\x72\x61\x63\x65\x2e\x62\
\x70\x66\x2e\x63\0\x5f\x5f\x73\x33\x32\x20\x78\x6d\x5f\x62\x70\x66\x5f\x64\x65\
\x6e\x79\x5f\x70\x74\x72\x61\x63\x65\x28\x73\x74\x72\x75\x63\x74\x20\x73\x79\
\x73\x63\x61\x6c\x6c\x73\x5f\x65\x6e\x74\x65\x72\x5f\x70\x74\x72\x61\x63\x65\
\x5f\x61\x72\x67\x73\x20\x2a\x63\x74\x78\x29\x20\x7b\0\x2f\x68\x6f\x6d\x65\x2f\
\x63\x61\x6c\x6d\x77\x75\x2f\x50\x72\x6f\x67\x72\x61\x6d\x2f\x78\x2d\x6d\x6f\
\x6e\x69\x74\x6f\x72\x2f\x70\x6c\x75\x67\x69\x6e\x5f\x65\x62\x70\x66\x2f\x62\
\x70\x66\x2f\x2e\x2f\x78\x6d\x5f\x62\x70\x66\x5f\x68\x65\x6c\x70\x65\x72\x73\
\x5f\x63\x6f\x6d\x6d\x6f\x6e\x2e\x68\0\x20\x20\x20\x20\x72\x65\x74\x75\x72\x6e\
\x20\x62\x70\x66\x5f\x67\x65\x74\x5f\x63\x75\x72\x72\x65\x6e\x74\x5f\x70\x69\
\x64\x5f\x74\x67\x69\x64\x28\x29\x20\x3e\x3e\x20\x33\x32\x3b\0\x20\x20\x20\x20\
\x69\x66\x20\x28\x74\x61\x72\x67\x65\x74\x5f\x70\x69\x64\x20\x21\x3d\x20\x30\
\x29\x20\x7b\0\x20\x20\x20\x20\x20\x20\x20\x20\x69\x66\x20\x28\x63\x74\x78\x2d\
\x3e\x70\x69\x64\x20\x21\x3d\x20\x74\x61\x72\x67\x65\x74\x5f\x70\x69\x64\x29\
\x20\x7b\0\x20\x20\x20\x20\x62\x70\x66\x5f\x70\x72\x69\x6e\x74\x6b\x28\x22\x78\
\x2d\x6d\x6f\x6e\x69\x74\x6f\x72\x20\x64\x65\x6e\x79\x20\x70\x69\x64\x3a\x27\
\x25\x6c\x64\x27\x20\x70\x74\x72\x61\x63\x65\x20\x74\x6f\x20\x74\x61\x72\x67\
\x65\x74\x20\x70\x69\x64\x3a\x27\x25\x6c\x64\x27\x22\x2c\x20\x70\x69\x64\x2c\0\
\x20\x20\x20\x20\x72\x65\x74\x20\x3d\x20\x62\x70\x66\x5f\x73\x65\x6e\x64\x5f\
\x73\x69\x67\x6e\x61\x6c\x28\x39\x29\x3b\0\x20\x20\x20\x20\x65\x76\x74\x20\x3d\
\x20\x62\x70\x66\x5f\x72\x69\x6e\x67\x62\x75\x66\x5f\x72\x65\x73\x65\x72\x76\
\x65\x28\x26\x78\x6d\x5f\x64\x70\x5f\x65\x76\x74\x5f\x72\x62\x2c\x20\x73\x69\
\x7a\x65\x6f\x66\x28\x2a\x65\x76\x74\x29\x2c\x20\x30\x29\x3b\0\x20\x20\x20\x20\
\x69\x66\x20\x28\x65\x76\x74\x29\x20\x7b\0\x20\x20\x20\x20\x20\x20\x20\x20\x65\
\x76\x74\x2d\x3e\x65\x72\x72\x5f\x63\x6f\x64\x65\x20\x3d\x20\x72\x65\x74\x3b\0\
\x20\x20\x20\x20\x20\x20\x20\x20\x65\x76\x74\x2d\x3e\x70\x69\x64\x20\x3d\x20\
\x70\x69\x64\x3b\0\x20\x20\x20\x20\x20\x20\x20\x20\x65\x76\x74\x2d\x3e\x70\x70\
\x69\x64\x20\x3d\x20\x30\x3b\0\x20\x20\x20\x20\x20\x20\x20\x20\x62\x70\x66\x5f\
\x67\x65\x74\x5f\x63\x75\x72\x72\x65\x6e\x74\x5f\x63\x6f\x6d\x6d\x28\x26\x65\
\x76\x74\x2d\x3e\x63\x6f\x6d\x6d\x2c\x20\x73\x69\x7a\x65\x6f\x66\x28\x65\x76\
\x74\x2d\x3e\x63\x6f\x6d\x6d\x29\x29\x3b\0\x20\x20\x20\x20\x20\x20\x20\x20\x62\
\x70\x66\x5f\x72\x69\x6e\x67\x62\x75\x66\x5f\x73\x75\x62\x6d\x69\x74\x28\x65\
\x76\x74\x2c\x20\x30\x29\x3b\0\x7d\0\x74\x61\x72\x67\x65\x74\x5f\x70\x69\x64\0\
\x63\x68\x61\x72\0\x78\x6d\x5f\x62\x70\x66\x5f\x64\x65\x6e\x79\x5f\x70\x74\x72\
\x61\x63\x65\x2e\x5f\x5f\x5f\x5f\x66\x6d\x74\0\x5f\x6c\x69\x63\x65\x6e\x73\x65\
\0\x2e\x6d\x61\x70\x73\0\x2e\x72\x6f\x64\x61\x74\x61\0\x6c\x69\x63\x65\x6e\x73\
\x65\0\x9f\xeb\x01\0\x20\0\0\0\0\0\0\0\x14\0\0\0\x14\0\0\0\x3c\x01\0\0\x50\x01\
\0\0\0\0\0\0\x08\0\0\0\xbd\0\0\0\x01\0\0\0\0\0\0\0\x11\0\0\0\x10\0\0\0\xbd\0\0\
\0\x13\0\0\0\0\0\0\0\xe2\0\0\0\x26\x01\0\0\0\x7c\0\0\x08\0\0\0\x69\x01\0\0\xb2\
\x01\0\0\x0c\xc4\x01\0\x18\0\0\0\xe2\0\0\0\xdf\x01\0\0\x09\x90\0\0\x30\0\0\0\
\xe2\0\0\0\xdf\x01\0\0\x09\x90\0\0\x38\0\0\0\xe2\0\0\0\xfa\x01\0\0\x12\x94\0\0\
\x40\0\0\0\xe2\0\0\0\xfa\x01\0\0\x19\x94\0\0\x48\0\0\0\xe2\0\0\0\xfa\x01\0\0\
\x0d\x94\0\0\x50\0\0\0\xe2\0\0\0\0\0\0\0\0\0\0\0\x58\0\0\0\xe2\0\0\0\x20\x02\0\
\0\x05\xa8\0\0\x88\0\0\0\xe2\0\0\0\x6b\x02\0\0\x0b\xb0\0\0\xa8\0\0\0\xe2\0\0\0\
\x89\x02\0\0\x0b\xc0\0\0\xd8\0\0\0\xe2\0\0\0\xc8\x02\0\0\x09\xc4\0\0\xe0\0\0\0\
\xe2\0\0\0\xd7\x02\0\0\x17\xd0\0\0\xe8\0\0\0\xe2\0\0\0\xf4\x02\0\0\x12\xc8\0\0\
\xf0\0\0\0\xe2\0\0\0\x0c\x03\0\0\x13\xcc\0\0\xf8\0\0\0\xe2\0\0\0\x23\x03\0\0\
\x24\xd4\0\0\x08\x01\0\0\xe2\0\0\0\x23\x03\0\0\x09\xd4\0\0\x18\x01\0\0\xe2\0\0\
\0\x60\x03\0\0\x09\xd8\0\0\x30\x01\0\0\xe2\0\0\0\x84\x03\0\0\x01\xe4\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\x03\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\xc8\0\0\0\0\0\x03\0\x50\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\xc1\0\0\0\0\0\x03\0\x30\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x14\0\0\0\x01\0\x05\0\
\x08\0\0\0\0\0\0\0\x34\0\0\0\0\0\0\0\0\0\0\0\x03\0\x05\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\x4c\0\0\0\x12\0\x03\0\0\0\0\0\0\0\0\0\x40\x01\0\0\0\0\0\0\x88\0\0\0\
\x11\0\x05\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\x93\0\0\0\x11\0\x06\0\0\0\0\0\0\
\0\0\0\x10\0\0\0\0\0\0\0\x43\0\0\0\x11\0\x07\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\
\0\x18\0\0\0\0\0\0\0\x01\0\0\0\x07\0\0\0\x60\0\0\0\0\0\0\0\x01\0\0\0\x05\0\0\0\
\xa8\0\0\0\0\0\0\0\x01\0\0\0\x08\0\0\0\x1c\x02\0\0\0\0\0\0\x04\0\0\0\x08\0\0\0\
\x34\x02\0\0\0\0\0\0\x03\0\0\0\x07\0\0\0\x40\x02\0\0\0\0\0\0\x03\0\0\0\x05\0\0\
\0\x58\x02\0\0\0\0\0\0\x04\0\0\0\x09\0\0\0\x2c\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\
\0\x40\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x50\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\
\x60\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x70\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\
\x80\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x90\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\
\xa0\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\xb0\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\
\xc0\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\xd0\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\
\xe0\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\xf0\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\0\
\x01\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x10\x01\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\
\x20\x01\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x30\x01\0\0\0\0\0\0\x04\0\0\0\x01\0\0\
\0\x40\x01\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x50\x01\0\0\0\0\0\0\x04\0\0\0\x01\0\
\0\0\x60\x01\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x0c\x0d\x05\x0e\x0f\0\x2e\x74\x65\
\x78\x74\0\x2e\x72\x65\x6c\x2e\x42\x54\x46\x2e\x65\x78\x74\0\x78\x6d\x5f\x62\
\x70\x66\x5f\x64\x65\x6e\x79\x5f\x70\x74\x72\x61\x63\x65\x2e\x5f\x5f\x5f\x5f\
\x66\x6d\x74\0\x2e\x6d\x61\x70\x73\0\x2e\x6c\x6c\x76\x6d\x5f\x61\x64\x64\x72\
\x73\x69\x67\0\x5f\x6c\x69\x63\x65\x6e\x73\x65\0\x78\x6d\x5f\x62\x70\x66\x5f\
\x64\x65\x6e\x79\x5f\x70\x74\x72\x61\x63\x65\0\x2e\x72\x65\x6c\x74\x72\x61\x63\
\x65\x70\x6f\x69\x6e\x74\x2f\x73\x79\x73\x63\x61\x6c\x6c\x73\x2f\x73\x79\x73\
\x5f\x65\x6e\x74\x65\x72\x5f\x70\x74\x72\x61\x63\x65\0\x74\x61\x72\x67\x65\x74\
\x5f\x70\x69\x64\0\x78\x6d\x5f\x64\x70\x5f\x65\x76\x74\x5f\x72\x62\0\x2e\x73\
\x74\x72\x74\x61\x62\0\x2e\x73\x79\x6d\x74\x61\x62\0\x2e\x72\x6f\x64\x61\x74\
\x61\0\x2e\x72\x65\x6c\x2e\x42\x54\x46\0\x4c\x42\x42\x30\x5f\x34\0\x4c\x42\x42\
\x30\x5f\x32\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xa0\0\
\0\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x1d\x0c\0\0\0\0\0\0\xcf\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\x01\0\0\0\
\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x63\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\0\0\x40\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\x5f\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\x68\x0a\0\0\0\0\0\0\x30\0\0\0\0\0\0\0\x0d\0\0\0\x03\0\0\0\x08\0\0\0\0\0\0\0\
\x10\0\0\0\0\0\0\0\xb0\0\0\0\x01\0\0\0\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x80\
\x01\0\0\0\0\0\0\x3c\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\x2f\0\0\0\x01\0\0\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xc0\x01\0\0\0\0\0\
\0\x10\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x44\0\0\
\0\x01\0\0\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xd0\x01\0\0\0\0\0\0\x04\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xbc\0\0\0\x01\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xd4\x01\0\0\0\0\0\0\x30\x06\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xb8\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\x98\x0a\0\0\0\0\0\0\x40\0\0\0\0\0\0\0\x0d\0\0\0\x08\0\0\0\
\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x0b\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\x04\x08\0\0\0\0\0\0\x70\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x07\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\xd8\x0a\0\0\0\0\0\0\x40\x01\0\0\0\0\0\0\x0d\0\0\0\x0a\0\0\0\x08\0\0\0\0\0\0\0\
\x10\0\0\0\0\0\0\0\x35\0\0\0\x03\x4c\xff\x6f\0\0\0\x80\0\0\0\0\0\0\0\0\0\0\0\0\
\x18\x0c\0\0\0\0\0\0\x05\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\xa8\0\0\0\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x78\x09\0\0\0\0\
\0\0\xf0\0\0\0\0\0\0\0\x01\0\0\0\x06\0\0\0\x08\0\0\0\0\0\0\0\x18\0\0\0\0\0\0\0";

	return 0;
err:
	bpf_object__destroy_skeleton(s);
	return -ENOMEM;
}

#endif /* __XM_DENY_PTRACE_BPF_SKEL_H__ */
