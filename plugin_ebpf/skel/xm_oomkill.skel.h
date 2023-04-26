/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */

/* THIS FILE IS AUTOGENERATED! */
#ifndef __XM_OOMKILL_BPF_SKEL_H__
#define __XM_OOMKILL_BPF_SKEL_H__

#include <errno.h>
#include <stdlib.h>
#include <bpf/libbpf.h>

struct xm_oomkill_bpf {
	struct bpf_object_skeleton *skeleton;
	struct bpf_object *obj;
	struct {
		struct bpf_map *xm_oomkill;
	} maps;
	struct {
		struct bpf_program *xm_oom_mark_victim;
	} progs;
	struct {
		struct bpf_link *xm_oom_mark_victim;
	} links;
};

static void
xm_oomkill_bpf__destroy(struct xm_oomkill_bpf *obj)
{
	if (!obj)
		return;
	if (obj->skeleton)
		bpf_object__destroy_skeleton(obj->skeleton);
	free(obj);
}

static inline int
xm_oomkill_bpf__create_skeleton(struct xm_oomkill_bpf *obj);

static inline struct xm_oomkill_bpf *
xm_oomkill_bpf__open_opts(const struct bpf_object_open_opts *opts)
{
	struct xm_oomkill_bpf *obj;
	int err;

	obj = (struct xm_oomkill_bpf *)calloc(1, sizeof(*obj));
	if (!obj) {
		errno = ENOMEM;
		return NULL;
	}

	err = xm_oomkill_bpf__create_skeleton(obj);
	err = err ?: bpf_object__open_skeleton(obj->skeleton, opts);
	if (err)
		goto err_out;

	return obj;
err_out:
	xm_oomkill_bpf__destroy(obj);
	errno = -err;
	return NULL;
}

static inline struct xm_oomkill_bpf *
xm_oomkill_bpf__open(void)
{
	return xm_oomkill_bpf__open_opts(NULL);
}

static inline int
xm_oomkill_bpf__load(struct xm_oomkill_bpf *obj)
{
	return bpf_object__load_skeleton(obj->skeleton);
}

static inline struct xm_oomkill_bpf *
xm_oomkill_bpf__open_and_load(void)
{
	struct xm_oomkill_bpf *obj;
	int err;

	obj = xm_oomkill_bpf__open();
	if (!obj)
		return NULL;
	err = xm_oomkill_bpf__load(obj);
	if (err) {
		xm_oomkill_bpf__destroy(obj);
		errno = -err;
		return NULL;
	}
	return obj;
}

static inline int
xm_oomkill_bpf__attach(struct xm_oomkill_bpf *obj)
{
	return bpf_object__attach_skeleton(obj->skeleton);
}

static inline void
xm_oomkill_bpf__detach(struct xm_oomkill_bpf *obj)
{
	return bpf_object__detach_skeleton(obj->skeleton);
}

static inline int
xm_oomkill_bpf__create_skeleton(struct xm_oomkill_bpf *obj)
{
	struct bpf_object_skeleton *s;

	s = (struct bpf_object_skeleton *)calloc(1, sizeof(*s));
	if (!s)
		goto err;
	obj->skeleton = s;

	s->sz = sizeof(*s);
	s->name = "xm_oomkill_bpf";
	s->obj = &obj->obj;

	/* maps */
	s->map_cnt = 1;
	s->map_skel_sz = sizeof(*s->maps);
	s->maps = (struct bpf_map_skeleton *)calloc(s->map_cnt, s->map_skel_sz);
	if (!s->maps)
		goto err;

	s->maps[0].name = "xm_oomkill";
	s->maps[0].map = &obj->maps.xm_oomkill;

	/* programs */
	s->prog_cnt = 1;
	s->prog_skel_sz = sizeof(*s->progs);
	s->progs = (struct bpf_prog_skeleton *)calloc(s->prog_cnt, s->prog_skel_sz);
	if (!s->progs)
		goto err;

	s->progs[0].name = "xm_oom_mark_victim";
	s->progs[0].prog = &obj->progs.xm_oom_mark_victim;
	s->progs[0].link = &obj->links.xm_oom_mark_victim;

	s->data_sz = 3264;
	s->data = (void *)"\
\x7f\x45\x4c\x46\x02\x01\x01\0\0\0\0\0\0\0\0\0\x01\0\xf7\0\x01\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x80\x09\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\x40\0\x0d\0\
\x01\0\x79\x16\0\0\0\0\0\0\xb7\x01\0\0\0\0\0\0\x7b\x1a\xf8\xff\0\0\0\0\x7b\x1a\
\xf0\xff\0\0\0\0\x7b\x1a\xe8\xff\0\0\0\0\x7b\x1a\xe0\xff\0\0\0\0\x7b\x1a\xd8\
\xff\0\0\0\0\x73\x1a\xd7\xff\0\0\0\0\xbf\xa1\0\0\0\0\0\0\x07\x01\0\0\xdc\xff\
\xff\xff\xb7\x02\0\0\x10\0\0\0\x85\0\0\0\x10\0\0\0\x61\x61\x08\0\0\0\0\0\x63\
\x1a\xd8\xff\0\0\0\0\xbf\xa2\0\0\0\0\0\0\x07\x02\0\0\xd8\xff\xff\xff\xbf\xa3\0\
\0\0\0\0\0\x07\x03\0\0\xd7\xff\xff\xff\x18\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xb7\
\x04\0\0\0\0\0\0\x85\0\0\0\x02\0\0\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\x47\
\x50\x4c\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\x9f\xeb\x01\0\x18\0\0\0\0\0\0\0\xa4\x02\0\0\xa4\x02\0\0\xab\x02\0\0\0\0\0\
\0\0\0\0\x02\x03\0\0\0\x01\0\0\0\0\0\0\x01\x04\0\0\0\x20\0\0\x01\0\0\0\0\0\0\0\
\x03\0\0\0\0\x02\0\0\0\x04\0\0\0\x05\0\0\0\x05\0\0\0\0\0\0\x01\x04\0\0\0\x20\0\
\0\0\0\0\0\0\0\0\0\x02\x06\0\0\0\0\0\0\0\0\0\0\x03\0\0\0\0\x02\0\0\0\x04\0\0\0\
\x40\0\0\0\0\0\0\0\0\0\0\x02\x08\0\0\0\x19\0\0\0\x04\0\0\x04\x28\0\0\0\x26\0\0\
\0\x09\0\0\0\0\0\0\0\x2a\0\0\0\x0c\0\0\0\x20\0\0\0\x2f\0\0\0\x0d\0\0\0\xc0\0\0\
\0\x39\0\0\0\x0f\0\0\0\0\x01\0\0\x44\0\0\0\0\0\0\x08\x0a\0\0\0\x4a\0\0\0\0\0\0\
\x08\x02\0\0\0\x59\0\0\0\0\0\0\x01\x01\0\0\0\x08\0\0\x01\0\0\0\0\0\0\0\x03\0\0\
\0\0\x0b\0\0\0\x04\0\0\0\x10\0\0\0\x5e\0\0\0\0\0\0\x08\x0e\0\0\0\x64\0\0\0\0\0\
\0\x01\x08\0\0\0\x40\0\0\0\x77\0\0\0\0\0\0\x08\x10\0\0\0\x7c\0\0\0\0\0\0\x01\
\x01\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\x02\x0f\0\0\0\0\0\0\0\x04\0\0\x04\x20\0\0\0\
\x8a\0\0\0\x01\0\0\0\0\0\0\0\x8f\0\0\0\x05\0\0\0\x40\0\0\0\x9b\0\0\0\x07\0\0\0\
\x80\0\0\0\x9f\0\0\0\x11\0\0\0\xc0\0\0\0\xa5\0\0\0\0\0\0\x0e\x12\0\0\0\x01\0\0\
\0\0\0\0\0\0\0\0\x02\x0e\0\0\0\0\0\0\0\x01\0\0\x0d\x16\0\0\0\xb0\0\0\0\x14\0\0\
\0\xb4\0\0\0\0\0\0\x08\x02\0\0\0\xba\0\0\0\x01\0\0\x0c\x15\0\0\0\xdc\x01\0\0\
\x03\0\0\x04\x0c\0\0\0\xf8\x01\0\0\x19\0\0\0\0\0\0\0\x26\0\0\0\x02\0\0\0\x40\0\
\0\0\xfc\x01\0\0\x1b\0\0\0\x60\0\0\0\x03\x02\0\0\x04\0\0\x04\x08\0\0\0\x8a\0\0\
\0\x1a\0\0\0\0\0\0\0\x0f\x02\0\0\x10\0\0\0\x10\0\0\0\x15\x02\0\0\x10\0\0\0\x18\
\0\0\0\x26\0\0\0\x02\0\0\0\x20\0\0\0\x23\x02\0\0\0\0\0\x01\x02\0\0\0\x10\0\0\0\
\0\0\0\0\0\0\0\x03\0\0\0\0\x0b\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\0\0\
\0\x0b\0\0\0\x04\0\0\0\x04\0\0\0\x94\x02\0\0\0\0\0\x0e\x1c\0\0\0\x01\0\0\0\x9d\
\x02\0\0\x01\0\0\x0f\0\0\0\0\x13\0\0\0\0\0\0\0\x20\0\0\0\xa3\x02\0\0\x01\0\0\
\x0f\0\0\0\0\x1d\0\0\0\0\0\0\0\x04\0\0\0\0\x69\x6e\x74\0\x5f\x5f\x41\x52\x52\
\x41\x59\x5f\x53\x49\x5a\x45\x5f\x54\x59\x50\x45\x5f\x5f\0\x6f\x6f\x6d\x6b\x69\
\x6c\x6c\x5f\x69\x6e\x66\x6f\0\x70\x69\x64\0\x63\x6f\x6d\x6d\0\x63\x67\x72\x6f\
\x75\x70\x5f\x69\x64\0\x67\x6c\x6f\x62\x61\x6c\x5f\x6f\x6f\x6d\0\x70\x69\x64\
\x5f\x74\0\x5f\x5f\x6b\x65\x72\x6e\x65\x6c\x5f\x70\x69\x64\x5f\x74\0\x63\x68\
\x61\x72\0\x5f\x5f\x75\x36\x34\0\x75\x6e\x73\x69\x67\x6e\x65\x64\x20\x6c\x6f\
\x6e\x67\x20\x6c\x6f\x6e\x67\0\x5f\x5f\x75\x38\0\x75\x6e\x73\x69\x67\x6e\x65\
\x64\x20\x63\x68\x61\x72\0\x74\x79\x70\x65\0\x6d\x61\x78\x5f\x65\x6e\x74\x72\
\x69\x65\x73\0\x6b\x65\x79\0\x76\x61\x6c\x75\x65\0\x78\x6d\x5f\x6f\x6f\x6d\x6b\
\x69\x6c\x6c\0\x63\x74\x78\0\x5f\x5f\x73\x33\x32\0\x78\x6d\x5f\x6f\x6f\x6d\x5f\
\x6d\x61\x72\x6b\x5f\x76\x69\x63\x74\x69\x6d\0\x74\x70\x2f\x6f\x6f\x6d\x2f\x6d\
\x61\x72\x6b\x5f\x76\x69\x63\x74\x69\x6d\0\x2f\x68\x6f\x6d\x65\x2f\x63\x61\x6c\
\x6d\x77\x75\x2f\x50\x72\x6f\x67\x72\x61\x6d\x2f\x78\x2d\x6d\x6f\x6e\x69\x74\
\x6f\x72\x2f\x70\x6c\x75\x67\x69\x6e\x5f\x65\x62\x70\x66\x2f\x62\x70\x66\x2f\
\x78\x6d\x5f\x6f\x6f\x6d\x6b\x69\x6c\x6c\x2e\x62\x70\x66\x2e\x63\0\x5f\x5f\x73\
\x33\x32\x20\x42\x50\x46\x5f\x50\x52\x4f\x47\x28\x78\x6d\x5f\x6f\x6f\x6d\x5f\
\x6d\x61\x72\x6b\x5f\x76\x69\x63\x74\x69\x6d\x2c\x20\x73\x74\x72\x75\x63\x74\
\x20\x74\x72\x61\x63\x65\x5f\x65\x76\x65\x6e\x74\x5f\x72\x61\x77\x5f\x6d\x61\
\x72\x6b\x5f\x76\x69\x63\x74\x69\x6d\x20\x2a\x65\x76\x74\x29\x20\x7b\0\x20\x20\
\x20\x20\x73\x74\x72\x75\x63\x74\x20\x6f\x6f\x6d\x6b\x69\x6c\x6c\x5f\x69\x6e\
\x66\x6f\x20\x69\x6e\x66\x6f\x20\x3d\x20\x7b\x7d\x3b\0\x20\x20\x20\x20\x5f\x5f\
\x75\x38\x20\x76\x61\x6c\x20\x3d\x20\x30\x3b\0\x20\x20\x20\x20\x62\x70\x66\x5f\
\x67\x65\x74\x5f\x63\x75\x72\x72\x65\x6e\x74\x5f\x63\x6f\x6d\x6d\x28\x26\x69\
\x6e\x66\x6f\x2e\x63\x6f\x6d\x6d\x2c\x20\x73\x69\x7a\x65\x6f\x66\x28\x69\x6e\
\x66\x6f\x2e\x63\x6f\x6d\x6d\x29\x29\x3b\0\x74\x72\x61\x63\x65\x5f\x65\x76\x65\
\x6e\x74\x5f\x72\x61\x77\x5f\x6d\x61\x72\x6b\x5f\x76\x69\x63\x74\x69\x6d\0\x65\
\x6e\x74\0\x5f\x5f\x64\x61\x74\x61\0\x74\x72\x61\x63\x65\x5f\x65\x6e\x74\x72\
\x79\0\x66\x6c\x61\x67\x73\0\x70\x72\x65\x65\x6d\x70\x74\x5f\x63\x6f\x75\x6e\
\x74\0\x75\x6e\x73\x69\x67\x6e\x65\x64\x20\x73\x68\x6f\x72\x74\0\x30\x3a\x31\0\
\x20\x20\x20\x20\x69\x6e\x66\x6f\x2e\x70\x69\x64\x20\x3d\x20\x28\x70\x69\x64\
\x5f\x74\x29\x28\x65\x76\x74\x2d\x3e\x70\x69\x64\x29\x3b\0\x20\x20\x20\x20\x62\
\x70\x66\x5f\x6d\x61\x70\x5f\x75\x70\x64\x61\x74\x65\x5f\x65\x6c\x65\x6d\x28\
\x26\x78\x6d\x5f\x6f\x6f\x6d\x6b\x69\x6c\x6c\x2c\x20\x26\x69\x6e\x66\x6f\x2c\
\x20\x26\x76\x61\x6c\x2c\x20\x42\x50\x46\x5f\x41\x4e\x59\x29\x3b\0\x5f\x6c\x69\
\x63\x65\x6e\x73\x65\0\x2e\x6d\x61\x70\x73\0\x6c\x69\x63\x65\x6e\x73\x65\0\0\
\x9f\xeb\x01\0\x20\0\0\0\0\0\0\0\x14\0\0\0\x14\0\0\0\xac\0\0\0\xc0\0\0\0\x1c\0\
\0\0\x08\0\0\0\xcd\0\0\0\x01\0\0\0\0\0\0\0\x17\0\0\0\x10\0\0\0\xcd\0\0\0\x0a\0\
\0\0\0\0\0\0\xe0\0\0\0\x20\x01\0\0\x07\x98\0\0\x10\0\0\0\xe0\0\0\0\x6e\x01\0\0\
\x19\x9c\0\0\x38\0\0\0\xe0\0\0\0\x91\x01\0\0\x0a\xa0\0\0\x40\0\0\0\xe0\0\0\0\
\xa3\x01\0\0\x1a\xa8\0\0\x50\0\0\0\xe0\0\0\0\xa3\x01\0\0\x05\xa8\0\0\x60\0\0\0\
\xe0\0\0\0\x36\x02\0\0\x1d\xac\0\0\x68\0\0\0\xe0\0\0\0\x36\x02\0\0\x0e\xac\0\0\
\x78\0\0\0\xe0\0\0\0\0\0\0\0\0\0\0\0\x90\0\0\0\xe0\0\0\0\x58\x02\0\0\x05\xb4\0\
\0\xb0\0\0\0\xe0\0\0\0\x20\x01\0\0\x07\x98\0\0\x10\0\0\0\xcd\0\0\0\x01\0\0\0\
\x60\0\0\0\x18\0\0\0\x32\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x1a\0\0\
\0\x12\0\x03\0\0\0\0\0\0\0\0\0\xc0\0\0\0\0\0\0\0\x44\0\0\0\x11\0\x06\0\0\0\0\0\
\0\0\0\0\x20\0\0\0\0\0\0\0\x5d\0\0\0\x11\0\x05\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\
\0\0\x90\0\0\0\0\0\0\0\x01\0\0\0\x03\0\0\0\x9c\x02\0\0\0\0\0\0\x04\0\0\0\x03\0\
\0\0\xb4\x02\0\0\0\0\0\0\x04\0\0\0\x04\0\0\0\x2c\0\0\0\0\0\0\0\x04\0\0\0\x01\0\
\0\0\x40\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x50\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\
\0\x60\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x70\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\
\x80\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x90\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\
\xa0\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\xb0\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\
\xc0\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\xd0\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\
\xec\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x09\x0b\x0a\0\x2e\x74\x65\x78\x74\0\x2e\
\x72\x65\x6c\x2e\x42\x54\x46\x2e\x65\x78\x74\0\x2e\x6d\x61\x70\x73\0\x78\x6d\
\x5f\x6f\x6f\x6d\x5f\x6d\x61\x72\x6b\x5f\x76\x69\x63\x74\x69\x6d\0\x2e\x72\x65\
\x6c\x74\x70\x2f\x6f\x6f\x6d\x2f\x6d\x61\x72\x6b\x5f\x76\x69\x63\x74\x69\x6d\0\
\x78\x6d\x5f\x6f\x6f\x6d\x6b\x69\x6c\x6c\0\x2e\x6c\x6c\x76\x6d\x5f\x61\x64\x64\
\x72\x73\x69\x67\0\x5f\x6c\x69\x63\x65\x6e\x73\x65\0\x2e\x73\x74\x72\x74\x61\
\x62\0\x2e\x73\x79\x6d\x74\x61\x62\0\x2e\x72\x65\x6c\x2e\x42\x54\x46\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x66\0\0\0\x03\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xfb\x08\0\0\0\0\0\0\x7f\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\x31\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x40\
\0\0\0\0\0\0\0\xc0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\x2d\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\x08\0\0\0\0\0\0\
\x10\0\0\0\0\0\0\0\x0c\0\0\0\x03\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x5e\
\0\0\0\x01\0\0\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\x04\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x14\0\0\0\x01\0\0\0\
\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\x01\0\0\0\0\0\0\x20\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x7a\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\x28\x01\0\0\0\0\0\0\x67\x05\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x76\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\x18\x08\0\0\0\0\0\0\x20\0\0\0\0\0\0\0\x0c\0\0\0\x07\0\0\0\x08\0\0\0\0\0\
\0\0\x10\0\0\0\0\0\0\0\x0b\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x90\
\x06\0\0\0\0\0\0\xfc\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\x07\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x38\x08\0\0\0\0\0\
\0\xc0\0\0\0\0\0\0\0\x0c\0\0\0\x09\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\
\x4f\0\0\0\x03\x4c\xff\x6f\0\0\0\x80\0\0\0\0\0\0\0\0\0\0\0\0\xf8\x08\0\0\0\0\0\
\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x6e\0\0\
\0\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x90\x07\0\0\0\0\0\0\x78\0\0\0\0\0\
\0\0\x01\0\0\0\x02\0\0\0\x08\0\0\0\0\0\0\0\x18\0\0\0\0\0\0\0";

	return 0;
err:
	bpf_object__destroy_skeleton(s);
	return -ENOMEM;
}

#endif /* __XM_OOMKILL_BPF_SKEL_H__ */
