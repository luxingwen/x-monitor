/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */

/* THIS FILE IS AUTOGENERATED! */
#ifndef __XM_CACHESTAT_BPF_SKEL_H__
#define __XM_CACHESTAT_BPF_SKEL_H__

#include <stdlib.h>
#include <bpf/libbpf.h>

struct xm_cachestat_bpf {
	struct bpf_object_skeleton *skeleton;
	struct bpf_object *obj;
	struct {
		struct bpf_map *bss;
	} maps;
	struct {
		struct bpf_program *xm_kp_cs_atpcl;
		struct bpf_program *xm_kp_cs_mpa;
		struct bpf_program *xm_kp_cs_apd;
		struct bpf_program *xm_kp_cs_mbd;
	} progs;
	struct {
		struct bpf_link *xm_kp_cs_atpcl;
		struct bpf_link *xm_kp_cs_mpa;
		struct bpf_link *xm_kp_cs_apd;
		struct bpf_link *xm_kp_cs_mbd;
	} links;
	struct xm_cachestat_bpf__bss {
		volatile __s64 xm_cs_total;
		volatile __s64 xm_cs_misses;
		volatile __s64 xm_cs_mbd;
	} *bss;
};

static void
xm_cachestat_bpf__destroy(struct xm_cachestat_bpf *obj)
{
	if (!obj)
		return;
	if (obj->skeleton)
		bpf_object__destroy_skeleton(obj->skeleton);
	free(obj);
}

static inline int
xm_cachestat_bpf__create_skeleton(struct xm_cachestat_bpf *obj);

static inline struct xm_cachestat_bpf *
xm_cachestat_bpf__open_opts(const struct bpf_object_open_opts *opts)
{
	struct xm_cachestat_bpf *obj;

	obj = (struct xm_cachestat_bpf *)calloc(1, sizeof(*obj));
	if (!obj)
		return NULL;
	if (xm_cachestat_bpf__create_skeleton(obj))
		goto err;
	if (bpf_object__open_skeleton(obj->skeleton, opts))
		goto err;

	return obj;
err:
	xm_cachestat_bpf__destroy(obj);
	return NULL;
}

static inline struct xm_cachestat_bpf *
xm_cachestat_bpf__open(void)
{
	return xm_cachestat_bpf__open_opts(NULL);
}

static inline int
xm_cachestat_bpf__load(struct xm_cachestat_bpf *obj)
{
	return bpf_object__load_skeleton(obj->skeleton);
}

static inline struct xm_cachestat_bpf *
xm_cachestat_bpf__open_and_load(void)
{
	struct xm_cachestat_bpf *obj;

	obj = xm_cachestat_bpf__open();
	if (!obj)
		return NULL;
	if (xm_cachestat_bpf__load(obj)) {
		xm_cachestat_bpf__destroy(obj);
		return NULL;
	}
	return obj;
}

static inline int
xm_cachestat_bpf__attach(struct xm_cachestat_bpf *obj)
{
	return bpf_object__attach_skeleton(obj->skeleton);
}

static inline void
xm_cachestat_bpf__detach(struct xm_cachestat_bpf *obj)
{
	return bpf_object__detach_skeleton(obj->skeleton);
}

static inline int
xm_cachestat_bpf__create_skeleton(struct xm_cachestat_bpf *obj)
{
	struct bpf_object_skeleton *s;

	s = (struct bpf_object_skeleton *)calloc(1, sizeof(*s));
	if (!s)
		return -1;
	obj->skeleton = s;

	s->sz = sizeof(*s);
	s->name = "xm_cachestat_bpf";
	s->obj = &obj->obj;

	/* maps */
	s->map_cnt = 1;
	s->map_skel_sz = sizeof(*s->maps);
	s->maps = (struct bpf_map_skeleton *)calloc(s->map_cnt, s->map_skel_sz);
	if (!s->maps)
		goto err;

	s->maps[0].name = "xm_cache.bss";
	s->maps[0].map = &obj->maps.bss;
	s->maps[0].mmaped = (void **)&obj->bss;

	/* programs */
	s->prog_cnt = 4;
	s->prog_skel_sz = sizeof(*s->progs);
	s->progs = (struct bpf_prog_skeleton *)calloc(s->prog_cnt, s->prog_skel_sz);
	if (!s->progs)
		goto err;

	s->progs[0].name = "xm_kp_cs_atpcl";
	s->progs[0].prog = &obj->progs.xm_kp_cs_atpcl;
	s->progs[0].link = &obj->links.xm_kp_cs_atpcl;

	s->progs[1].name = "xm_kp_cs_mpa";
	s->progs[1].prog = &obj->progs.xm_kp_cs_mpa;
	s->progs[1].link = &obj->links.xm_kp_cs_mpa;

	s->progs[2].name = "xm_kp_cs_apd";
	s->progs[2].prog = &obj->progs.xm_kp_cs_apd;
	s->progs[2].link = &obj->links.xm_kp_cs_apd;

	s->progs[3].name = "xm_kp_cs_mbd";
	s->progs[3].prog = &obj->progs.xm_kp_cs_mbd;
	s->progs[3].link = &obj->links.xm_kp_cs_mbd;

	s->data_sz = 4376;
	s->data = (void *)"\
\x7f\x45\x4c\x46\x02\x01\x01\0\0\0\0\0\0\0\0\0\x01\0\xf7\0\x01\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x58\x0c\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\x40\0\x13\0\
\x01\0\xb7\x01\0\0\x01\0\0\0\x18\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xdb\x12\0\0\0\
\0\0\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\xb7\x01\0\0\x01\0\0\0\x18\x02\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\xdb\x12\0\0\0\0\0\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\
\xb7\x01\0\0\xff\xff\xff\xff\x18\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xdb\x12\0\0\0\
\0\0\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\xb7\x01\0\0\xff\xff\xff\xff\x18\x02\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xdb\x12\0\0\0\0\0\0\xb7\x01\0\0\x01\0\0\0\x18\x02\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xdb\x12\0\0\0\0\0\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\
\0\0\0\x47\x50\x4c\0\x9f\xeb\x01\0\x18\0\0\0\0\0\0\0\xa8\x02\0\0\xa8\x02\0\0\
\x17\x03\0\0\0\0\0\0\0\0\0\x02\x02\0\0\0\x01\0\0\0\x15\0\0\x04\xa8\0\0\0\x09\0\
\0\0\x03\0\0\0\0\0\0\0\x0d\0\0\0\x03\0\0\0\x40\0\0\0\x11\0\0\0\x03\0\0\0\x80\0\
\0\0\x15\0\0\0\x03\0\0\0\xc0\0\0\0\x19\0\0\0\x03\0\0\0\0\x01\0\0\x1c\0\0\0\x03\
\0\0\0\x40\x01\0\0\x1f\0\0\0\x03\0\0\0\x80\x01\0\0\x23\0\0\0\x03\0\0\0\xc0\x01\
\0\0\x27\0\0\0\x03\0\0\0\0\x02\0\0\x2a\0\0\0\x03\0\0\0\x40\x02\0\0\x2d\0\0\0\
\x03\0\0\0\x80\x02\0\0\x30\0\0\0\x03\0\0\0\xc0\x02\0\0\x33\0\0\0\x03\0\0\0\0\
\x03\0\0\x36\0\0\0\x03\0\0\0\x40\x03\0\0\x39\0\0\0\x03\0\0\0\x80\x03\0\0\x3c\0\
\0\0\x03\0\0\0\xc0\x03\0\0\x44\0\0\0\x03\0\0\0\0\x04\0\0\x47\0\0\0\x03\0\0\0\
\x40\x04\0\0\x4a\0\0\0\x03\0\0\0\x80\x04\0\0\x50\0\0\0\x03\0\0\0\xc0\x04\0\0\
\x53\0\0\0\x03\0\0\0\0\x05\0\0\x56\0\0\0\0\0\0\x01\x08\0\0\0\x40\0\0\0\0\0\0\0\
\x01\0\0\x0d\x05\0\0\0\x64\0\0\0\x01\0\0\0\x68\0\0\0\0\0\0\x08\x06\0\0\0\x6e\0\
\0\0\0\0\0\x01\x04\0\0\0\x20\0\0\x01\x72\0\0\0\x01\0\0\x0c\x04\0\0\0\0\0\0\0\
\x01\0\0\x0d\x05\0\0\0\x64\0\0\0\x01\0\0\0\x2f\x01\0\0\x01\0\0\x0c\x08\0\0\0\0\
\0\0\0\x01\0\0\x0d\x05\0\0\0\x64\0\0\0\x01\0\0\0\xa2\x01\0\0\x01\0\0\x0c\x0a\0\
\0\0\0\0\0\0\x01\0\0\x0d\x05\0\0\0\x64\0\0\0\x01\0\0\0\x19\x02\0\0\x01\0\0\x0c\
\x0c\0\0\0\0\0\0\0\0\0\0\x09\x0f\0\0\0\xb5\x02\0\0\0\0\0\x08\x10\0\0\0\xbb\x02\
\0\0\0\0\0\x01\x08\0\0\0\x40\0\0\x01\xc5\x02\0\0\0\0\0\x0e\x0e\0\0\0\x01\0\0\0\
\xd1\x02\0\0\0\0\0\x0e\x0e\0\0\0\x01\0\0\0\xde\x02\0\0\0\0\0\x0e\x0e\0\0\0\x01\
\0\0\0\xe8\x02\0\0\0\0\0\x01\x01\0\0\0\x08\0\0\x01\0\0\0\0\0\0\0\x03\0\0\0\0\
\x14\0\0\0\x16\0\0\0\x04\0\0\0\xed\x02\0\0\0\0\0\x01\x04\0\0\0\x20\0\0\0\x01\
\x03\0\0\0\0\0\x0e\x15\0\0\0\x01\0\0\0\x0a\x03\0\0\x03\0\0\x0f\0\0\0\0\x11\0\0\
\0\0\0\0\0\x08\0\0\0\x12\0\0\0\0\0\0\0\x08\0\0\0\x13\0\0\0\0\0\0\0\x08\0\0\0\
\x0f\x03\0\0\x01\0\0\x0f\0\0\0\0\x17\0\0\0\0\0\0\0\x04\0\0\0\0\x70\x74\x5f\x72\
\x65\x67\x73\0\x72\x31\x35\0\x72\x31\x34\0\x72\x31\x33\0\x72\x31\x32\0\x62\x70\
\0\x62\x78\0\x72\x31\x31\0\x72\x31\x30\0\x72\x39\0\x72\x38\0\x61\x78\0\x63\x78\
\0\x64\x78\0\x73\x69\0\x64\x69\0\x6f\x72\x69\x67\x5f\x61\x78\0\x69\x70\0\x63\
\x73\0\x66\x6c\x61\x67\x73\0\x73\x70\0\x73\x73\0\x75\x6e\x73\x69\x67\x6e\x65\
\x64\x20\x6c\x6f\x6e\x67\0\x63\x74\x78\0\x5f\x5f\x73\x33\x32\0\x69\x6e\x74\0\
\x78\x6d\x5f\x6b\x70\x5f\x63\x73\x5f\x61\x74\x70\x63\x6c\0\x6b\x70\x72\x6f\x62\
\x65\x2f\x61\x64\x64\x5f\x74\x6f\x5f\x70\x61\x67\x65\x5f\x63\x61\x63\x68\x65\
\x5f\x6c\x72\x75\0\x2f\x68\x6f\x6d\x65\x2f\x63\x61\x6c\x6d\x77\x75\x2f\x50\x72\
\x6f\x67\x72\x61\x6d\x2f\x78\x2d\x6d\x6f\x6e\x69\x74\x6f\x72\x2f\x70\x6c\x75\
\x67\x69\x6e\x5f\x65\x62\x70\x66\x2f\x62\x70\x66\x2f\x78\x6d\x5f\x63\x61\x63\
\x68\x65\x73\x74\x61\x74\x2e\x62\x70\x66\x2e\x63\0\x5f\x5f\x73\x33\x32\x20\x42\
\x50\x46\x5f\x4b\x50\x52\x4f\x42\x45\x28\x78\x6d\x5f\x6b\x70\x5f\x63\x73\x5f\
\x61\x74\x70\x63\x6c\x29\x20\x7b\0\x20\x20\x20\x20\x5f\x5f\x73\x79\x6e\x63\x5f\
\x66\x65\x74\x63\x68\x5f\x61\x6e\x64\x5f\x61\x64\x64\x28\x26\x78\x6d\x5f\x63\
\x73\x5f\x6d\x69\x73\x73\x65\x73\x2c\x20\x31\x29\x3b\0\x78\x6d\x5f\x6b\x70\x5f\
\x63\x73\x5f\x6d\x70\x61\0\x6b\x70\x72\x6f\x62\x65\x2f\x6d\x61\x72\x6b\x5f\x70\
\x61\x67\x65\x5f\x61\x63\x63\x65\x73\x73\x65\x64\0\x5f\x5f\x73\x33\x32\x20\x42\
\x50\x46\x5f\x4b\x50\x52\x4f\x42\x45\x28\x78\x6d\x5f\x6b\x70\x5f\x63\x73\x5f\
\x6d\x70\x61\x29\x20\x7b\0\x20\x20\x20\x20\x5f\x5f\x73\x79\x6e\x63\x5f\x66\x65\
\x74\x63\x68\x5f\x61\x6e\x64\x5f\x61\x64\x64\x28\x26\x78\x6d\x5f\x63\x73\x5f\
\x74\x6f\x74\x61\x6c\x2c\x20\x31\x29\x3b\0\x78\x6d\x5f\x6b\x70\x5f\x63\x73\x5f\
\x61\x70\x64\0\x6b\x70\x72\x6f\x62\x65\x2f\x61\x63\x63\x6f\x75\x6e\x74\x5f\x70\
\x61\x67\x65\x5f\x64\x69\x72\x74\x69\x65\x64\0\x5f\x5f\x73\x33\x32\x20\x42\x50\
\x46\x5f\x4b\x50\x52\x4f\x42\x45\x28\x78\x6d\x5f\x6b\x70\x5f\x63\x73\x5f\x61\
\x70\x64\x29\x20\x7b\0\x20\x20\x20\x20\x5f\x5f\x73\x79\x6e\x63\x5f\x66\x65\x74\
\x63\x68\x5f\x61\x6e\x64\x5f\x61\x64\x64\x28\x26\x78\x6d\x5f\x63\x73\x5f\x6d\
\x69\x73\x73\x65\x73\x2c\x20\x2d\x31\x29\x3b\0\x78\x6d\x5f\x6b\x70\x5f\x63\x73\
\x5f\x6d\x62\x64\0\x6b\x70\x72\x6f\x62\x65\x2f\x6d\x61\x72\x6b\x5f\x62\x75\x66\
\x66\x65\x72\x5f\x64\x69\x72\x74\x79\0\x5f\x5f\x73\x33\x32\x20\x42\x50\x46\x5f\
\x4b\x50\x52\x4f\x42\x45\x28\x78\x6d\x5f\x6b\x70\x5f\x63\x73\x5f\x6d\x62\x64\
\x29\x20\x7b\0\x20\x20\x20\x20\x5f\x5f\x73\x79\x6e\x63\x5f\x66\x65\x74\x63\x68\
\x5f\x61\x6e\x64\x5f\x61\x64\x64\x28\x26\x78\x6d\x5f\x63\x73\x5f\x74\x6f\x74\
\x61\x6c\x2c\x20\x2d\x31\x29\x3b\0\x20\x20\x20\x20\x5f\x5f\x73\x79\x6e\x63\x5f\
\x66\x65\x74\x63\x68\x5f\x61\x6e\x64\x5f\x61\x64\x64\x28\x26\x78\x6d\x5f\x63\
\x73\x5f\x6d\x62\x64\x2c\x20\x31\x29\x3b\0\x5f\x5f\x73\x36\x34\0\x6c\x6f\x6e\
\x67\x20\x6c\x6f\x6e\x67\0\x78\x6d\x5f\x63\x73\x5f\x74\x6f\x74\x61\x6c\0\x78\
\x6d\x5f\x63\x73\x5f\x6d\x69\x73\x73\x65\x73\0\x78\x6d\x5f\x63\x73\x5f\x6d\x62\
\x64\0\x63\x68\x61\x72\0\x5f\x5f\x41\x52\x52\x41\x59\x5f\x53\x49\x5a\x45\x5f\
\x54\x59\x50\x45\x5f\x5f\0\x5f\x6c\x69\x63\x65\x6e\x73\x65\0\x2e\x62\x73\x73\0\
\x6c\x69\x63\x65\x6e\x73\x65\0\0\x9f\xeb\x01\0\x20\0\0\0\0\0\0\0\x44\0\0\0\x44\
\0\0\0\xf4\0\0\0\x38\x01\0\0\0\0\0\0\x08\0\0\0\x81\0\0\0\x01\0\0\0\0\0\0\0\x07\
\0\0\0\x3c\x01\0\0\x01\0\0\0\0\0\0\0\x09\0\0\0\xaf\x01\0\0\x01\0\0\0\0\0\0\0\
\x0b\0\0\0\x26\x02\0\0\x01\0\0\0\0\0\0\0\x0d\0\0\0\x10\0\0\0\x81\0\0\0\x03\0\0\
\0\0\0\0\0\x9e\0\0\0\xe0\0\0\0\0\x54\0\0\x08\0\0\0\x9e\0\0\0\x03\x01\0\0\x05\
\x58\0\0\x20\0\0\0\x9e\0\0\0\xe0\0\0\0\x07\x54\0\0\x3c\x01\0\0\x03\0\0\0\0\0\0\
\0\x9e\0\0\0\x56\x01\0\0\0\x6c\0\0\x08\0\0\0\x9e\0\0\0\x77\x01\0\0\x05\x70\0\0\
\x20\0\0\0\x9e\0\0\0\x56\x01\0\0\x07\x6c\0\0\xaf\x01\0\0\x03\0\0\0\0\0\0\0\x9e\
\0\0\0\xcb\x01\0\0\0\xa0\0\0\x08\0\0\0\x9e\0\0\0\xec\x01\0\0\x05\xa4\0\0\x20\0\
\0\0\x9e\0\0\0\xcb\x01\0\0\x07\xa0\0\0\x26\x02\0\0\x04\0\0\0\0\0\0\0\x9e\0\0\0\
\x3f\x02\0\0\0\xbc\0\0\x08\0\0\0\x9e\0\0\0\x60\x02\0\0\x05\xc0\0\0\x28\0\0\0\
\x9e\0\0\0\x8c\x02\0\0\x05\xc4\0\0\x40\0\0\0\x9e\0\0\0\x3f\x02\0\0\x07\xbc\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\x03\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\x05\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\x03\0\x07\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\x09\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x64\0\0\0\x12\0\x03\0\0\0\0\0\0\0\0\0\x30\0\0\0\
\0\0\0\0\x57\0\0\0\x11\0\x0b\0\x08\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\x08\x01\0\0\
\x12\0\x05\0\0\0\0\0\0\0\0\0\x30\0\0\0\0\0\0\0\x73\0\0\0\x11\0\x0b\0\0\0\0\0\0\
\0\0\0\x08\0\0\0\0\0\0\0\x96\0\0\0\x12\0\x07\0\0\0\0\0\0\0\0\0\x30\0\0\0\0\0\0\
\0\xe1\0\0\0\x12\0\x09\0\0\0\0\0\0\0\0\0\x50\0\0\0\0\0\0\0\xee\0\0\0\x11\0\x0b\
\0\x10\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\x8d\0\0\0\x11\0\x0c\0\0\0\0\0\0\0\0\0\
\x04\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\x01\0\0\0\x06\0\0\0\x08\0\0\0\0\0\0\0\x01\
\0\0\0\x08\0\0\0\x08\0\0\0\0\0\0\0\x01\0\0\0\x06\0\0\0\x08\0\0\0\0\0\0\0\x01\0\
\0\0\x08\0\0\0\x28\0\0\0\0\0\0\0\x01\0\0\0\x0b\0\0\0\x88\x02\0\0\0\0\0\0\x04\0\
\0\0\x08\0\0\0\x94\x02\0\0\0\0\0\0\x04\0\0\0\x06\0\0\0\xa0\x02\0\0\0\0\0\0\x04\
\0\0\0\x0b\0\0\0\xb8\x02\0\0\0\0\0\0\x04\0\0\0\x0c\0\0\0\x2c\0\0\0\0\0\0\0\x04\
\0\0\0\x01\0\0\0\x3c\0\0\0\0\0\0\0\x04\0\0\0\x02\0\0\0\x4c\0\0\0\0\0\0\0\x04\0\
\0\0\x03\0\0\0\x5c\0\0\0\0\0\0\0\x04\0\0\0\x04\0\0\0\x70\0\0\0\0\0\0\0\x04\0\0\
\0\x01\0\0\0\x80\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x90\0\0\0\0\0\0\0\x04\0\0\0\
\x01\0\0\0\xa8\0\0\0\0\0\0\0\x04\0\0\0\x02\0\0\0\xb8\0\0\0\0\0\0\0\x04\0\0\0\
\x02\0\0\0\xc8\0\0\0\0\0\0\0\x04\0\0\0\x02\0\0\0\xe0\0\0\0\0\0\0\0\x04\0\0\0\
\x03\0\0\0\xf0\0\0\0\0\0\0\0\x04\0\0\0\x03\0\0\0\0\x01\0\0\0\0\0\0\x04\0\0\0\
\x03\0\0\0\x18\x01\0\0\0\0\0\0\x04\0\0\0\x04\0\0\0\x28\x01\0\0\0\0\0\0\x04\0\0\
\0\x04\0\0\0\x38\x01\0\0\0\0\0\0\x04\0\0\0\x04\0\0\0\x48\x01\0\0\0\0\0\0\x04\0\
\0\0\x04\0\0\0\x0b\x0d\x0f\x10\x0e\x0c\x11\x12\0\x2e\x72\x65\x6c\x6b\x70\x72\
\x6f\x62\x65\x2f\x6d\x61\x72\x6b\x5f\x62\x75\x66\x66\x65\x72\x5f\x64\x69\x72\
\x74\x79\0\x2e\x72\x65\x6c\x6b\x70\x72\x6f\x62\x65\x2f\x61\x64\x64\x5f\x74\x6f\
\x5f\x70\x61\x67\x65\x5f\x63\x61\x63\x68\x65\x5f\x6c\x72\x75\0\x2e\x74\x65\x78\
\x74\0\x2e\x72\x65\x6c\x2e\x42\x54\x46\x2e\x65\x78\x74\0\x2e\x62\x73\x73\0\x78\
\x6d\x5f\x63\x73\x5f\x6d\x69\x73\x73\x65\x73\0\x78\x6d\x5f\x6b\x70\x5f\x63\x73\
\x5f\x61\x74\x70\x63\x6c\0\x78\x6d\x5f\x63\x73\x5f\x74\x6f\x74\x61\x6c\0\x2e\
\x6c\x6c\x76\x6d\x5f\x61\x64\x64\x72\x73\x69\x67\0\x5f\x6c\x69\x63\x65\x6e\x73\
\x65\0\x78\x6d\x5f\x6b\x70\x5f\x63\x73\x5f\x61\x70\x64\0\x2e\x72\x65\x6c\x6b\
\x70\x72\x6f\x62\x65\x2f\x6d\x61\x72\x6b\x5f\x70\x61\x67\x65\x5f\x61\x63\x63\
\x65\x73\x73\x65\x64\0\x2e\x72\x65\x6c\x6b\x70\x72\x6f\x62\x65\x2f\x61\x63\x63\
\x6f\x75\x6e\x74\x5f\x70\x61\x67\x65\x5f\x64\x69\x72\x74\x69\x65\x64\0\x78\x6d\
\x5f\x6b\x70\x5f\x63\x73\x5f\x6d\x62\x64\0\x78\x6d\x5f\x63\x73\x5f\x6d\x62\x64\
\0\x2e\x73\x74\x72\x74\x61\x62\0\x2e\x73\x79\x6d\x74\x61\x62\0\x78\x6d\x5f\x6b\
\x70\x5f\x63\x73\x5f\x6d\x70\x61\0\x2e\x72\x65\x6c\x2e\x42\x54\x46\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xf8\0\0\0\x03\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\x38\x0b\0\0\0\0\0\0\x1e\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x3f\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\x22\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\
\0\0\0\x30\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x1e\
\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x90\x09\0\0\0\0\0\0\x10\0\0\
\0\0\0\0\0\x12\0\0\0\x03\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\xa7\0\0\0\
\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x70\0\0\0\0\0\0\0\x30\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xa3\0\0\0\x09\0\0\0\x40\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xa0\x09\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x12\0\0\0\
\x05\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\xc5\0\0\0\x01\0\0\0\x06\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\xa0\0\0\0\0\0\0\0\x30\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xc1\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\xb0\x09\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x12\0\0\0\x07\0\0\0\x08\0\0\0\0\0\
\0\0\x10\0\0\0\0\0\0\0\x05\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\xd0\0\0\0\0\0\0\0\x50\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\x01\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xc0\x09\0\0\0\0\
\0\0\x20\0\0\0\0\0\0\0\x12\0\0\0\x09\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\
\x52\0\0\0\x08\0\0\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x20\x01\0\0\0\0\0\0\x18\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x8e\0\0\0\x01\
\0\0\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x20\x01\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x19\x01\0\0\x01\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\x24\x01\0\0\0\0\0\0\xd7\x05\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x15\x01\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\xe0\x09\0\0\0\0\0\0\x40\0\0\0\0\0\0\0\x12\0\0\0\x0d\0\0\0\x08\
\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x49\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\xfc\x06\0\0\0\0\0\0\x58\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\x45\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x20\
\x0a\0\0\0\0\0\0\x10\x01\0\0\0\0\0\0\x12\0\0\0\x0f\0\0\0\x08\0\0\0\0\0\0\0\x10\
\0\0\0\0\0\0\0\x7f\0\0\0\x03\x4c\xff\x6f\0\0\0\x80\0\0\0\0\0\0\0\0\0\0\0\0\x30\
\x0b\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\x01\0\0\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x58\x08\0\0\0\0\0\0\
\x38\x01\0\0\0\0\0\0\x01\0\0\0\x05\0\0\0\x08\0\0\0\0\0\0\0\x18\0\0\0\0\0\0\0";

	return 0;
err:
	bpf_object__destroy_skeleton(s);
	return -1;
}

#endif /* __XM_CACHESTAT_BPF_SKEL_H__ */
