/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */

/* THIS FILE IS AUTOGENERATED! */
#ifndef __XM_MISC_BPF_SKEL_H__
#define __XM_MISC_BPF_SKEL_H__

#include <errno.h>
#include <stdlib.h>
#include <bpf/libbpf.h>

struct xm_misc_bpf {
	struct bpf_object_skeleton *skeleton;
	struct bpf_object *obj;
	struct {
		struct bpf_map *rodata;
	} maps;
	struct {
		struct bpf_program *hello;
		struct bpf_program *do_unlinkat;
		struct bpf_program *do_unlinkat_exit;
	} progs;
	struct {
		struct bpf_link *hello;
		struct bpf_link *do_unlinkat;
		struct bpf_link *do_unlinkat_exit;
	} links;
	struct xm_misc_bpf__rodata {
	} *rodata;
};

static void
xm_misc_bpf__destroy(struct xm_misc_bpf *obj)
{
	if (!obj)
		return;
	if (obj->skeleton)
		bpf_object__destroy_skeleton(obj->skeleton);
	free(obj);
}

static inline int
xm_misc_bpf__create_skeleton(struct xm_misc_bpf *obj);

static inline struct xm_misc_bpf *
xm_misc_bpf__open_opts(const struct bpf_object_open_opts *opts)
{
	struct xm_misc_bpf *obj;
	int err;

	obj = (struct xm_misc_bpf *)calloc(1, sizeof(*obj));
	if (!obj) {
		errno = ENOMEM;
		return NULL;
	}

	err = xm_misc_bpf__create_skeleton(obj);
	err = err ?: bpf_object__open_skeleton(obj->skeleton, opts);
	if (err)
		goto err_out;

	return obj;
err_out:
	xm_misc_bpf__destroy(obj);
	errno = -err;
	return NULL;
}

static inline struct xm_misc_bpf *
xm_misc_bpf__open(void)
{
	return xm_misc_bpf__open_opts(NULL);
}

static inline int
xm_misc_bpf__load(struct xm_misc_bpf *obj)
{
	return bpf_object__load_skeleton(obj->skeleton);
}

static inline struct xm_misc_bpf *
xm_misc_bpf__open_and_load(void)
{
	struct xm_misc_bpf *obj;
	int err;

	obj = xm_misc_bpf__open();
	if (!obj)
		return NULL;
	err = xm_misc_bpf__load(obj);
	if (err) {
		xm_misc_bpf__destroy(obj);
		errno = -err;
		return NULL;
	}
	return obj;
}

static inline int
xm_misc_bpf__attach(struct xm_misc_bpf *obj)
{
	return bpf_object__attach_skeleton(obj->skeleton);
}

static inline void
xm_misc_bpf__detach(struct xm_misc_bpf *obj)
{
	return bpf_object__detach_skeleton(obj->skeleton);
}

static inline int
xm_misc_bpf__create_skeleton(struct xm_misc_bpf *obj)
{
	struct bpf_object_skeleton *s;

	s = (struct bpf_object_skeleton *)calloc(1, sizeof(*s));
	if (!s)
		goto err;
	obj->skeleton = s;

	s->sz = sizeof(*s);
	s->name = "xm_misc_bpf";
	s->obj = &obj->obj;

	/* maps */
	s->map_cnt = 1;
	s->map_skel_sz = sizeof(*s->maps);
	s->maps = (struct bpf_map_skeleton *)calloc(s->map_cnt, s->map_skel_sz);
	if (!s->maps)
		goto err;

	s->maps[0].name = "xm_misc_.rodata";
	s->maps[0].map = &obj->maps.rodata;
	s->maps[0].mmaped = (void **)&obj->rodata;

	/* programs */
	s->prog_cnt = 3;
	s->prog_skel_sz = sizeof(*s->progs);
	s->progs = (struct bpf_prog_skeleton *)calloc(s->prog_cnt, s->prog_skel_sz);
	if (!s->progs)
		goto err;

	s->progs[0].name = "hello";
	s->progs[0].prog = &obj->progs.hello;
	s->progs[0].link = &obj->links.hello;

	s->progs[1].name = "do_unlinkat";
	s->progs[1].prog = &obj->progs.do_unlinkat;
	s->progs[1].link = &obj->links.do_unlinkat;

	s->progs[2].name = "do_unlinkat_exit";
	s->progs[2].prog = &obj->progs.do_unlinkat_exit;
	s->progs[2].link = &obj->links.do_unlinkat_exit;

	s->data_sz = 5160;
	s->data = (void *)"\
\x7f\x45\x4c\x46\x02\x01\x01\0\0\0\0\0\0\0\0\0\x01\0\xf7\0\x01\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\xe8\x0f\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\x40\0\x11\0\
\x01\0\x79\x10\x08\0\0\0\0\0\x95\0\0\0\0\0\0\0\x85\x10\0\0\xff\xff\xff\xff\x18\
\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xb7\x02\0\0\x0c\0\0\0\xbf\x03\0\0\0\0\0\0\x85\
\0\0\0\x06\0\0\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\x79\x16\x68\0\0\0\0\0\x85\
\0\0\0\x0e\0\0\0\xbf\x07\0\0\0\0\0\0\xb7\x01\0\0\0\0\0\0\x0f\x16\0\0\0\0\0\0\
\xbf\xa1\0\0\0\0\0\0\x07\x01\0\0\xf8\xff\xff\xff\xb7\x02\0\0\x08\0\0\0\xbf\x63\
\0\0\0\0\0\0\x85\0\0\0\x71\0\0\0\x79\xa4\xf8\xff\0\0\0\0\x77\x07\0\0\x20\0\0\0\
\x18\x01\0\0\x0c\0\0\0\0\0\0\0\0\0\0\0\xb7\x02\0\0\x26\0\0\0\xbf\x73\0\0\0\0\0\
\0\x85\0\0\0\x06\0\0\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\x79\x16\x50\0\0\0\0\
\0\x85\0\0\0\x0e\0\0\0\x77\0\0\0\x20\0\0\0\x18\x01\0\0\x32\0\0\0\0\0\0\0\0\0\0\
\0\xb7\x02\0\0\x22\0\0\0\xbf\x03\0\0\0\0\0\0\xbf\x64\0\0\0\0\0\0\x85\0\0\0\x06\
\0\0\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\x44\x75\x61\x6c\x20\x42\x53\x44\x2f\
\x47\x50\x4c\0\x53\x79\x73\x63\x61\x6c\x6c\x3a\x20\x25\x64\0\x4b\x50\x52\x4f\
\x42\x45\x20\x45\x4e\x54\x52\x59\x20\x70\x69\x64\x20\x3d\x20\x25\x64\x2c\x20\
\x66\x69\x6c\x65\x6e\x61\x6d\x65\x20\x3d\x20\x25\x73\x0a\0\x4b\x50\x52\x4f\x42\
\x45\x20\x45\x58\x49\x54\x3a\x20\x70\x69\x64\x20\x3d\x20\x25\x64\x2c\x20\x72\
\x65\x74\x20\x3d\x20\x25\x6c\x64\x0a\0\0\0\0\x9f\xeb\x01\0\x18\0\0\0\0\0\0\0\
\xb0\x03\0\0\xb0\x03\0\0\xc3\x03\0\0\0\0\0\0\0\0\0\x02\x02\0\0\0\x01\0\0\0\x01\
\0\0\x04\0\0\0\0\x19\0\0\0\x05\0\0\0\0\0\0\0\x1e\0\0\0\0\0\0\x08\x04\0\0\0\x24\
\0\0\0\0\0\0\x01\x08\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\x03\0\0\0\0\x03\0\0\0\x06\0\
\0\0\0\0\0\0\x37\0\0\0\0\0\0\x01\x04\0\0\0\x20\0\0\0\0\0\0\0\x01\0\0\x0d\x08\0\
\0\0\x4b\0\0\0\x01\0\0\0\x4f\0\0\0\0\0\0\x08\x09\0\0\0\x55\0\0\0\0\0\0\x01\x04\
\0\0\0\x20\0\0\x01\x59\0\0\0\x01\0\0\x0c\x07\0\0\0\0\0\0\0\x01\0\0\x0d\x08\0\0\
\0\x4b\0\0\0\x01\0\0\0\xfd\0\0\0\0\0\0\x0c\x0b\0\0\0\0\0\0\0\0\0\0\x02\x0e\0\0\
\0\x2d\x01\0\0\x15\0\0\x04\xa8\0\0\0\x35\x01\0\0\x0f\0\0\0\0\0\0\0\x39\x01\0\0\
\x0f\0\0\0\x40\0\0\0\x3d\x01\0\0\x0f\0\0\0\x80\0\0\0\x41\x01\0\0\x0f\0\0\0\xc0\
\0\0\0\x45\x01\0\0\x0f\0\0\0\0\x01\0\0\x48\x01\0\0\x0f\0\0\0\x40\x01\0\0\x4b\
\x01\0\0\x0f\0\0\0\x80\x01\0\0\x4f\x01\0\0\x0f\0\0\0\xc0\x01\0\0\x53\x01\0\0\
\x0f\0\0\0\0\x02\0\0\x56\x01\0\0\x0f\0\0\0\x40\x02\0\0\x59\x01\0\0\x0f\0\0\0\
\x80\x02\0\0\x5c\x01\0\0\x0f\0\0\0\xc0\x02\0\0\x5f\x01\0\0\x0f\0\0\0\0\x03\0\0\
\x62\x01\0\0\x0f\0\0\0\x40\x03\0\0\x65\x01\0\0\x0f\0\0\0\x80\x03\0\0\x68\x01\0\
\0\x0f\0\0\0\xc0\x03\0\0\x70\x01\0\0\x0f\0\0\0\0\x04\0\0\x73\x01\0\0\x0f\0\0\0\
\x40\x04\0\0\x76\x01\0\0\x0f\0\0\0\x80\x04\0\0\x7c\x01\0\0\x0f\0\0\0\xc0\x04\0\
\0\x7f\x01\0\0\x0f\0\0\0\0\x05\0\0\x82\x01\0\0\0\0\0\x01\x08\0\0\0\x40\0\0\0\0\
\0\0\0\x01\0\0\x0d\x09\0\0\0\x4b\0\0\0\x0d\0\0\0\x90\x01\0\0\x01\0\0\x0c\x10\0\
\0\0\x1e\x02\0\0\x05\0\0\x04\x20\0\0\0\x27\x02\0\0\x13\0\0\0\0\0\0\0\x2c\x02\0\
\0\x13\0\0\0\x40\0\0\0\x31\x02\0\0\x09\0\0\0\x80\0\0\0\x38\x02\0\0\x16\0\0\0\
\xc0\0\0\0\x3e\x02\0\0\x17\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\x02\x14\0\0\0\0\0\0\0\
\0\0\0\x0a\x15\0\0\0\x44\x02\0\0\0\0\0\x01\x01\0\0\0\x08\0\0\x01\0\0\0\0\0\0\0\
\x02\x24\0\0\0\0\0\0\0\0\0\0\x03\0\0\0\0\x14\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\
\x01\0\0\x0d\x09\0\0\0\x4b\0\0\0\x0d\0\0\0\xc0\x02\0\0\x01\0\0\x0c\x18\0\0\0\0\
\0\0\0\0\0\0\x03\0\0\0\0\x15\0\0\0\x06\0\0\0\x0d\0\0\0\x5c\x03\0\0\0\0\0\x0e\
\x1a\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\x03\0\0\0\0\x14\0\0\0\x06\0\0\0\x0c\0\0\0\
\x64\x03\0\0\0\0\0\x0e\x1c\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\0\0\0\x14\0\0\0\
\x06\0\0\0\x26\0\0\0\x72\x03\0\0\0\0\0\x0e\x1e\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\
\0\0\0\0\x14\0\0\0\x06\0\0\0\x22\0\0\0\x8a\x03\0\0\0\0\0\x0e\x20\0\0\0\0\0\0\0\
\xa7\x03\0\0\x03\0\0\x0f\0\0\0\0\x1d\0\0\0\0\0\0\0\x0c\0\0\0\x1f\0\0\0\x0c\0\0\
\0\x26\0\0\0\x21\0\0\0\x32\0\0\0\x22\0\0\0\xaf\x03\0\0\x01\0\0\x0f\0\0\0\0\x1b\
\0\0\0\0\0\0\0\x0d\0\0\0\xb7\x03\0\0\0\0\0\x07\0\0\0\0\0\x62\x70\x66\x5f\x72\
\x61\x77\x5f\x74\x72\x61\x63\x65\x70\x6f\x69\x6e\x74\x5f\x61\x72\x67\x73\0\x61\
\x72\x67\x73\0\x5f\x5f\x75\x36\x34\0\x75\x6e\x73\x69\x67\x6e\x65\x64\x20\x6c\
\x6f\x6e\x67\x20\x6c\x6f\x6e\x67\0\x5f\x5f\x41\x52\x52\x41\x59\x5f\x53\x49\x5a\
\x45\x5f\x54\x59\x50\x45\x5f\x5f\0\x63\x74\x78\0\x5f\x5f\x73\x33\x32\0\x69\x6e\
\x74\0\x68\x65\x6c\x6c\x6f\0\x72\x61\x77\x5f\x74\x70\x2f\0\x2f\x68\x6f\x6d\x65\
\x2f\x63\x61\x6c\x6d\x77\x75\x2f\x50\x72\x6f\x67\x72\x61\x6d\x2f\x78\x2d\x6d\
\x6f\x6e\x69\x74\x6f\x72\x2f\x70\x6c\x75\x67\x69\x6e\x5f\x65\x62\x70\x66\x2f\
\x62\x70\x66\x2f\x78\x6d\x5f\x6d\x69\x73\x63\x2e\x62\x70\x66\x2e\x63\0\x20\x20\
\x20\x20\x5f\x5f\x73\x33\x32\x20\x6f\x70\x63\x6f\x64\x65\x20\x3d\x20\x67\x65\
\x74\x5f\x6f\x70\x63\x6f\x64\x65\x28\x63\x74\x78\x29\x3b\0\x20\x20\x20\x20\x62\
\x70\x66\x5f\x70\x72\x69\x6e\x74\x6b\x28\x22\x53\x79\x73\x63\x61\x6c\x6c\x3a\
\x20\x25\x64\x22\x2c\x20\x6f\x70\x63\x6f\x64\x65\x29\x3b\0\x20\x20\x20\x20\x72\
\x65\x74\x75\x72\x6e\x20\x30\x3b\0\x67\x65\x74\x5f\x6f\x70\x63\x6f\x64\x65\0\
\x2e\x74\x65\x78\x74\0\x30\x3a\x30\x3a\x31\0\x20\x20\x20\x20\x72\x65\x74\x75\
\x72\x6e\x20\x63\x74\x78\x2d\x3e\x61\x72\x67\x73\x5b\x31\x5d\x3b\0\x70\x74\x5f\
\x72\x65\x67\x73\0\x72\x31\x35\0\x72\x31\x34\0\x72\x31\x33\0\x72\x31\x32\0\x62\
\x70\0\x62\x78\0\x72\x31\x31\0\x72\x31\x30\0\x72\x39\0\x72\x38\0\x61\x78\0\x63\
\x78\0\x64\x78\0\x73\x69\0\x64\x69\0\x6f\x72\x69\x67\x5f\x61\x78\0\x69\x70\0\
\x63\x73\0\x66\x6c\x61\x67\x73\0\x73\x70\0\x73\x73\0\x75\x6e\x73\x69\x67\x6e\
\x65\x64\x20\x6c\x6f\x6e\x67\0\x64\x6f\x5f\x75\x6e\x6c\x69\x6e\x6b\x61\x74\0\
\x6b\x70\x72\x6f\x62\x65\x2f\x64\x6f\x5f\x75\x6e\x6c\x69\x6e\x6b\x61\x74\0\x30\
\x3a\x31\x33\0\x69\x6e\x74\x20\x42\x50\x46\x5f\x4b\x50\x52\x4f\x42\x45\x28\x64\
\x6f\x5f\x75\x6e\x6c\x69\x6e\x6b\x61\x74\x2c\x20\x69\x6e\x74\x20\x64\x66\x64\
\x2c\x20\x73\x74\x72\x75\x63\x74\x20\x66\x69\x6c\x65\x6e\x61\x6d\x65\x20\x2a\
\x6e\x61\x6d\x65\x29\x20\x7b\0\x20\x20\x20\x20\x70\x69\x64\x20\x3d\x20\x62\x70\
\x66\x5f\x67\x65\x74\x5f\x63\x75\x72\x72\x65\x6e\x74\x5f\x70\x69\x64\x5f\x74\
\x67\x69\x64\x28\x29\x20\x3e\x3e\x20\x33\x32\x3b\0\x66\x69\x6c\x65\x6e\x61\x6d\
\x65\0\x6e\x61\x6d\x65\0\x75\x70\x74\x72\0\x72\x65\x66\x63\x6e\x74\0\x61\x6e\
\x61\x6d\x65\0\x69\x6e\x61\x6d\x65\0\x63\x68\x61\x72\0\x30\x3a\x30\0\x20\x20\
\x20\x20\x66\x69\x6c\x65\x6e\x61\x6d\x65\x20\x3d\x20\x42\x50\x46\x5f\x43\x4f\
\x52\x45\x5f\x52\x45\x41\x44\x28\x6e\x61\x6d\x65\x2c\x20\x6e\x61\x6d\x65\x29\
\x3b\0\x20\x20\x20\x20\x62\x70\x66\x5f\x70\x72\x69\x6e\x74\x6b\x28\x22\x4b\x50\
\x52\x4f\x42\x45\x20\x45\x4e\x54\x52\x59\x20\x70\x69\x64\x20\x3d\x20\x25\x64\
\x2c\x20\x66\x69\x6c\x65\x6e\x61\x6d\x65\x20\x3d\x20\x25\x73\x5c\x6e\x22\x2c\
\x20\x70\x69\x64\x2c\x20\x66\x69\x6c\x65\x6e\x61\x6d\x65\x29\x3b\0\x64\x6f\x5f\
\x75\x6e\x6c\x69\x6e\x6b\x61\x74\x5f\x65\x78\x69\x74\0\x6b\x72\x65\x74\x70\x72\
\x6f\x62\x65\x2f\x64\x6f\x5f\x75\x6e\x6c\x69\x6e\x6b\x61\x74\0\x30\x3a\x31\x30\
\0\x69\x6e\x74\x20\x42\x50\x46\x5f\x4b\x52\x45\x54\x50\x52\x4f\x42\x45\x28\x64\
\x6f\x5f\x75\x6e\x6c\x69\x6e\x6b\x61\x74\x5f\x65\x78\x69\x74\x2c\x20\x6c\x6f\
\x6e\x67\x20\x72\x65\x74\x29\x20\x7b\0\x20\x20\x20\x20\x62\x70\x66\x5f\x70\x72\
\x69\x6e\x74\x6b\x28\x22\x4b\x50\x52\x4f\x42\x45\x20\x45\x58\x49\x54\x3a\x20\
\x70\x69\x64\x20\x3d\x20\x25\x64\x2c\x20\x72\x65\x74\x20\x3d\x20\x25\x6c\x64\
\x5c\x6e\x22\x2c\x20\x70\x69\x64\x2c\x20\x72\x65\x74\x29\x3b\0\x4c\x49\x43\x45\
\x4e\x53\x45\0\x68\x65\x6c\x6c\x6f\x2e\x5f\x5f\x5f\x5f\x66\x6d\x74\0\x5f\x5f\
\x5f\x5f\x64\x6f\x5f\x75\x6e\x6c\x69\x6e\x6b\x61\x74\x2e\x5f\x5f\x5f\x5f\x66\
\x6d\x74\0\x5f\x5f\x5f\x5f\x64\x6f\x5f\x75\x6e\x6c\x69\x6e\x6b\x61\x74\x5f\x65\
\x78\x69\x74\x2e\x5f\x5f\x5f\x5f\x66\x6d\x74\0\x2e\x72\x6f\x64\x61\x74\x61\0\
\x6c\x69\x63\x65\x6e\x73\x65\0\x61\x75\x64\x69\x74\x5f\x6e\x61\x6d\x65\x73\0\0\
\x9f\xeb\x01\0\x20\0\0\0\0\0\0\0\x44\0\0\0\x44\0\0\0\x44\x01\0\0\x88\x01\0\0\
\x5c\0\0\0\x08\0\0\0\x5f\0\0\0\x01\0\0\0\0\0\0\0\x0a\0\0\0\x08\x01\0\0\x01\0\0\
\0\0\0\0\0\x0c\0\0\0\x9c\x01\0\0\x01\0\0\0\0\0\0\0\x11\0\0\0\xd1\x02\0\0\x01\0\
\0\0\0\0\0\0\x19\0\0\0\x10\0\0\0\x5f\0\0\0\x03\0\0\0\0\0\0\0\x67\0\0\0\xa4\0\0\
\0\x14\x50\0\0\x08\0\0\0\x67\0\0\0\xc8\0\0\0\x05\x54\0\0\x30\0\0\0\x67\0\0\0\
\xef\0\0\0\x05\x58\0\0\x08\x01\0\0\x02\0\0\0\0\0\0\0\x67\0\0\0\x14\x01\0\0\x0c\
\x3c\0\0\x08\0\0\0\x67\0\0\0\x14\x01\0\0\x05\x3c\0\0\x9c\x01\0\0\x08\0\0\0\0\0\
\0\0\x67\0\0\0\xb4\x01\0\0\x05\x68\0\0\x08\0\0\0\x67\0\0\0\xf2\x01\0\0\x0b\x78\
\0\0\x30\0\0\0\x67\0\0\0\0\0\0\0\0\0\0\0\x38\0\0\0\x67\0\0\0\x4d\x02\0\0\x10\
\x7c\0\0\x50\0\0\0\x67\0\0\0\x4d\x02\0\0\x10\x7c\0\0\x58\0\0\0\x67\0\0\0\xf2\
\x01\0\0\x26\x78\0\0\x60\0\0\0\x67\0\0\0\x77\x02\0\0\x05\x80\0\0\x88\0\0\0\x67\
\0\0\0\xb4\x01\0\0\x05\x68\0\0\xd1\x02\0\0\x05\0\0\0\0\0\0\0\x67\0\0\0\xec\x02\
\0\0\x05\x94\0\0\x08\0\0\0\x67\0\0\0\xf2\x01\0\0\x0b\xa0\0\0\x10\0\0\0\x67\0\0\
\0\xf2\x01\0\0\x26\xa0\0\0\x18\0\0\0\x67\0\0\0\x1c\x03\0\0\x05\xa4\0\0\x48\0\0\
\0\x67\0\0\0\xec\x02\0\0\x05\x94\0\0\x10\0\0\0\x08\x01\0\0\x01\0\0\0\0\0\0\0\
\x02\0\0\0\x0e\x01\0\0\0\0\0\0\x9c\x01\0\0\x02\0\0\0\0\0\0\0\x0e\0\0\0\xaf\x01\
\0\0\0\0\0\0\x18\0\0\0\x12\0\0\0\x49\x02\0\0\0\0\0\0\xd1\x02\0\0\x01\0\0\0\0\0\
\0\0\x0e\0\0\0\xe7\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\x03\0\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\
\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xb5\0\0\0\x02\0\x02\0\0\0\0\0\0\0\0\0\
\x10\0\0\0\0\0\0\0\x49\0\0\0\x01\0\x0a\0\0\0\0\0\0\0\0\0\x0c\0\0\0\0\0\0\0\0\0\
\0\0\x03\0\x05\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x31\0\0\0\x01\0\x0a\0\x0c\0\0\
\0\0\0\0\0\x26\0\0\0\0\0\0\0\0\0\0\0\x03\0\x07\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\x14\0\0\0\x01\0\x0a\0\x32\0\0\0\0\0\0\0\x22\0\0\0\0\0\0\0\0\0\0\0\x03\0\x0a\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x99\0\0\0\x12\0\x03\0\0\0\0\0\0\0\0\0\x40\0\
\0\0\0\0\0\0\x8d\0\0\0\x12\0\x05\0\0\0\0\0\0\0\0\0\x98\0\0\0\0\0\0\0\x57\0\0\0\
\x12\0\x07\0\0\0\0\0\0\0\0\0\x58\0\0\0\0\0\0\0\xe1\0\0\0\x11\0\x09\0\0\0\0\0\0\
\0\0\0\x0d\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x0a\0\0\0\x01\0\0\0\x08\0\0\0\0\0\0\0\
\x01\0\0\0\x09\0\0\0\x60\0\0\0\0\0\0\0\x01\0\0\0\x09\0\0\0\x18\0\0\0\0\0\0\0\
\x01\0\0\0\x09\0\0\0\x84\x03\0\0\0\0\0\0\x03\0\0\0\x09\0\0\0\x90\x03\0\0\0\0\0\
\0\x03\0\0\0\x09\0\0\0\x9c\x03\0\0\0\0\0\0\x03\0\0\0\x09\0\0\0\xb4\x03\0\0\0\0\
\0\0\x04\0\0\0\x0d\0\0\0\x2c\0\0\0\0\0\0\0\x04\0\0\0\x02\0\0\0\x3c\0\0\0\0\0\0\
\0\x04\0\0\0\x01\0\0\0\x4c\0\0\0\0\0\0\0\x04\0\0\0\x05\0\0\0\x5c\0\0\0\0\0\0\0\
\x04\0\0\0\x07\0\0\0\x70\0\0\0\0\0\0\0\x04\0\0\0\x02\0\0\0\x80\0\0\0\0\0\0\0\
\x04\0\0\0\x02\0\0\0\x90\0\0\0\0\0\0\0\x04\0\0\0\x02\0\0\0\xa8\0\0\0\0\0\0\0\
\x04\0\0\0\x01\0\0\0\xb8\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\xd0\0\0\0\0\0\0\0\
\x04\0\0\0\x05\0\0\0\xe0\0\0\0\0\0\0\0\x04\0\0\0\x05\0\0\0\xf0\0\0\0\0\0\0\0\
\x04\0\0\0\x05\0\0\0\0\x01\0\0\0\0\0\0\x04\0\0\0\x05\0\0\0\x10\x01\0\0\0\0\0\0\
\x04\0\0\0\x05\0\0\0\x20\x01\0\0\0\0\0\0\x04\0\0\0\x05\0\0\0\x30\x01\0\0\0\0\0\
\0\x04\0\0\0\x05\0\0\0\x40\x01\0\0\0\0\0\0\x04\0\0\0\x05\0\0\0\x58\x01\0\0\0\0\
\0\0\x04\0\0\0\x07\0\0\0\x68\x01\0\0\0\0\0\0\x04\0\0\0\x07\0\0\0\x78\x01\0\0\0\
\0\0\0\x04\0\0\0\x07\0\0\0\x88\x01\0\0\0\0\0\0\x04\0\0\0\x07\0\0\0\x98\x01\0\0\
\0\0\0\0\x04\0\0\0\x07\0\0\0\xb4\x01\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\xcc\x01\0\
\0\0\0\0\0\x04\0\0\0\x05\0\0\0\xdc\x01\0\0\0\0\0\0\x04\0\0\0\x05\0\0\0\xf4\x01\
\0\0\0\0\0\0\x04\0\0\0\x07\0\0\0\x11\x12\x13\x14\x05\x07\x09\0\x2e\x74\x65\x78\
\x74\0\x2e\x72\x65\x6c\x2e\x42\x54\x46\x2e\x65\x78\x74\0\x5f\x5f\x5f\x5f\x64\
\x6f\x5f\x75\x6e\x6c\x69\x6e\x6b\x61\x74\x5f\x65\x78\x69\x74\x2e\x5f\x5f\x5f\
\x5f\x66\x6d\x74\0\x5f\x5f\x5f\x5f\x64\x6f\x5f\x75\x6e\x6c\x69\x6e\x6b\x61\x74\
\x2e\x5f\x5f\x5f\x5f\x66\x6d\x74\0\x68\x65\x6c\x6c\x6f\x2e\x5f\x5f\x5f\x5f\x66\
\x6d\x74\0\x64\x6f\x5f\x75\x6e\x6c\x69\x6e\x6b\x61\x74\x5f\x65\x78\x69\x74\0\
\x2e\x72\x65\x6c\x6b\x72\x65\x74\x70\x72\x6f\x62\x65\x2f\x64\x6f\x5f\x75\x6e\
\x6c\x69\x6e\x6b\x61\x74\0\x2e\x72\x65\x6c\x6b\x70\x72\x6f\x62\x65\x2f\x64\x6f\
\x5f\x75\x6e\x6c\x69\x6e\x6b\x61\x74\0\x68\x65\x6c\x6c\x6f\0\x2e\x6c\x6c\x76\
\x6d\x5f\x61\x64\x64\x72\x73\x69\x67\0\x6c\x69\x63\x65\x6e\x73\x65\0\x67\x65\
\x74\x5f\x6f\x70\x63\x6f\x64\x65\0\x2e\x73\x74\x72\x74\x61\x62\0\x2e\x73\x79\
\x6d\x74\x61\x62\0\x2e\x72\x6f\x64\x61\x74\x61\0\x2e\x72\x65\x6c\x2e\x42\x54\
\x46\0\x4c\x49\x43\x45\x4e\x53\x45\0\x2e\x72\x65\x6c\x72\x61\x77\x5f\x74\x70\
\x2f\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xc0\0\0\0\x03\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xef\x0e\0\0\0\0\0\0\xf5\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\x01\0\0\0\x06\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xed\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\x50\0\0\0\0\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\xe9\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xc8\
\x0c\0\0\0\0\0\0\x20\0\0\0\0\0\0\0\x10\0\0\0\x03\0\0\0\x08\0\0\0\0\0\0\0\x10\0\
\0\0\0\0\0\0\x86\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x90\0\0\0\0\
\0\0\0\x98\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x82\
\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xe8\x0c\0\0\0\0\0\0\x10\0\0\
\0\0\0\0\0\x10\0\0\0\x05\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x6c\0\0\0\
\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x28\x01\0\0\0\0\0\0\x58\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x68\0\0\0\x09\0\0\0\x40\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xf8\x0c\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x10\0\0\0\
\x07\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\xad\0\0\0\x01\0\0\0\x03\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x80\x01\0\0\0\0\0\0\x0d\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xd0\0\0\0\x01\0\0\0\x02\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\x8d\x01\0\0\0\0\0\0\x54\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\xdc\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xe4\
\x01\0\0\0\0\0\0\x8b\x07\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\xd8\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\x0d\0\0\0\0\
\0\0\x40\0\0\0\0\0\0\0\x10\0\0\0\x0b\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\
\x0b\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x70\x09\0\0\0\0\0\0\x04\
\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x07\0\0\0\
\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x48\x0d\0\0\0\0\0\0\xa0\x01\0\0\0\
\0\0\0\x10\0\0\0\x0d\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x9f\0\0\0\x03\
\x4c\xff\x6f\0\0\0\x80\0\0\0\0\0\0\0\0\0\0\0\0\xe8\x0e\0\0\0\0\0\0\x07\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xc8\0\0\0\x02\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x78\x0b\0\0\0\0\0\0\x50\x01\0\0\0\0\0\0\x01\0\0\
\0\x0a\0\0\0\x08\0\0\0\0\0\0\0\x18\0\0\0\0\0\0\0";

	return 0;
err:
	bpf_object__destroy_skeleton(s);
	return -ENOMEM;
}

#endif /* __XM_MISC_BPF_SKEL_H__ */