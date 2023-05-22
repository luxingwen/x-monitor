/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */

/* THIS FILE IS AUTOGENERATED! */
#ifndef __XM_BIOLAT_BPF_SKEL_H__
#define __XM_BIOLAT_BPF_SKEL_H__

#include <errno.h>
#include <stdlib.h>
#include <bpf/libbpf.h>

struct xm_biolat_bpf {
	struct bpf_object_skeleton *skeleton;
	struct bpf_object *obj;
	struct {
		struct bpf_program *xm_tp_btf__block_rq_insert;
		struct bpf_program *xm_tp_btf__block_rq_issue;
		struct bpf_program *xm_tp_btf__block_rq_complete;
	} progs;
	struct {
		struct bpf_link *xm_tp_btf__block_rq_insert;
		struct bpf_link *xm_tp_btf__block_rq_issue;
		struct bpf_link *xm_tp_btf__block_rq_complete;
	} links;
};

static void
xm_biolat_bpf__destroy(struct xm_biolat_bpf *obj)
{
	if (!obj)
		return;
	if (obj->skeleton)
		bpf_object__destroy_skeleton(obj->skeleton);
	free(obj);
}

static inline int
xm_biolat_bpf__create_skeleton(struct xm_biolat_bpf *obj);

static inline struct xm_biolat_bpf *
xm_biolat_bpf__open_opts(const struct bpf_object_open_opts *opts)
{
	struct xm_biolat_bpf *obj;
	int err;

	obj = (struct xm_biolat_bpf *)calloc(1, sizeof(*obj));
	if (!obj) {
		errno = ENOMEM;
		return NULL;
	}

	err = xm_biolat_bpf__create_skeleton(obj);
	err = err ?: bpf_object__open_skeleton(obj->skeleton, opts);
	if (err)
		goto err_out;

	return obj;
err_out:
	xm_biolat_bpf__destroy(obj);
	errno = -err;
	return NULL;
}

static inline struct xm_biolat_bpf *
xm_biolat_bpf__open(void)
{
	return xm_biolat_bpf__open_opts(NULL);
}

static inline int
xm_biolat_bpf__load(struct xm_biolat_bpf *obj)
{
	return bpf_object__load_skeleton(obj->skeleton);
}

static inline struct xm_biolat_bpf *
xm_biolat_bpf__open_and_load(void)
{
	struct xm_biolat_bpf *obj;
	int err;

	obj = xm_biolat_bpf__open();
	if (!obj)
		return NULL;
	err = xm_biolat_bpf__load(obj);
	if (err) {
		xm_biolat_bpf__destroy(obj);
		errno = -err;
		return NULL;
	}
	return obj;
}

static inline int
xm_biolat_bpf__attach(struct xm_biolat_bpf *obj)
{
	return bpf_object__attach_skeleton(obj->skeleton);
}

static inline void
xm_biolat_bpf__detach(struct xm_biolat_bpf *obj)
{
	return bpf_object__detach_skeleton(obj->skeleton);
}

static inline int
xm_biolat_bpf__create_skeleton(struct xm_biolat_bpf *obj)
{
	struct bpf_object_skeleton *s;

	s = (struct bpf_object_skeleton *)calloc(1, sizeof(*s));
	if (!s)
		goto err;
	obj->skeleton = s;

	s->sz = sizeof(*s);
	s->name = "xm_biolat_bpf";
	s->obj = &obj->obj;

	/* programs */
	s->prog_cnt = 3;
	s->prog_skel_sz = sizeof(*s->progs);
	s->progs = (struct bpf_prog_skeleton *)calloc(s->prog_cnt, s->prog_skel_sz);
	if (!s->progs)
		goto err;

	s->progs[0].name = "xm_tp_btf__block_rq_insert";
	s->progs[0].prog = &obj->progs.xm_tp_btf__block_rq_insert;
	s->progs[0].link = &obj->links.xm_tp_btf__block_rq_insert;

	s->progs[1].name = "xm_tp_btf__block_rq_issue";
	s->progs[1].prog = &obj->progs.xm_tp_btf__block_rq_issue;
	s->progs[1].link = &obj->links.xm_tp_btf__block_rq_issue;

	s->progs[2].name = "xm_tp_btf__block_rq_complete";
	s->progs[2].prog = &obj->progs.xm_tp_btf__block_rq_complete;
	s->progs[2].link = &obj->links.xm_tp_btf__block_rq_complete;

	s->data_sz = 2432;
	s->data = (void *)"\
\x7f\x45\x4c\x46\x02\x01\x01\0\0\0\0\0\0\0\0\0\x01\0\xf7\0\x01\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x40\x06\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\x40\0\x0d\0\
\x01\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\
\xb7\0\0\0\0\0\0\0\x95\0\0\0\0\0\0\0\x44\x75\x61\x6c\x20\x42\x53\x44\x2f\x47\
\x50\x4c\0\0\0\0\x9f\xeb\x01\0\x18\0\0\0\0\0\0\0\xf8\0\0\0\xf8\0\0\0\xf7\x01\0\
\0\0\0\0\0\0\0\0\x02\x02\0\0\0\x01\0\0\0\0\0\0\x01\x08\0\0\0\x40\0\0\0\0\0\0\0\
\x01\0\0\x0d\x04\0\0\0\x14\0\0\0\x01\0\0\0\x18\0\0\0\0\0\0\x08\x05\0\0\0\x1e\0\
\0\0\0\0\0\x01\x04\0\0\0\x20\0\0\x01\x22\0\0\0\x01\0\0\x0c\x03\0\0\0\0\0\0\0\
\x01\0\0\x0d\x04\0\0\0\x14\0\0\0\x01\0\0\0\xd7\0\0\0\x01\0\0\x0c\x07\0\0\0\0\0\
\0\0\x01\0\0\x0d\x04\0\0\0\x14\0\0\0\x01\0\0\0\x4a\x01\0\0\x01\0\0\x0c\x09\0\0\
\0\xce\x01\0\0\0\0\0\x01\x01\0\0\0\x08\0\0\x01\0\0\0\0\0\0\0\x03\0\0\0\0\x0b\0\
\0\0\x0d\0\0\0\x0d\0\0\0\xd3\x01\0\0\0\0\0\x01\x04\0\0\0\x20\0\0\0\xe7\x01\0\0\
\0\0\0\x0e\x0c\0\0\0\x01\0\0\0\xef\x01\0\0\x01\0\0\x0f\0\0\0\0\x0e\0\0\0\0\0\0\
\0\x0d\0\0\0\0\x75\x6e\x73\x69\x67\x6e\x65\x64\x20\x6c\x6f\x6e\x67\x20\x6c\x6f\
\x6e\x67\0\x63\x74\x78\0\x5f\x5f\x73\x33\x32\0\x69\x6e\x74\0\x78\x6d\x5f\x74\
\x70\x5f\x62\x74\x66\x5f\x5f\x62\x6c\x6f\x63\x6b\x5f\x72\x71\x5f\x69\x6e\x73\
\x65\x72\x74\0\x74\x70\x5f\x62\x74\x66\x2f\x62\x6c\x6f\x63\x6b\x5f\x72\x71\x5f\
\x69\x6e\x73\x65\x72\x74\0\x2f\x68\x6f\x6d\x65\x2f\x63\x61\x6c\x6d\x77\x75\x2f\
\x50\x72\x6f\x67\x72\x61\x6d\x2f\x78\x2d\x6d\x6f\x6e\x69\x74\x6f\x72\x2f\x70\
\x6c\x75\x67\x69\x6e\x5f\x65\x62\x70\x66\x2f\x62\x70\x66\x2f\x78\x6d\x5f\x62\
\x69\x6f\x6c\x61\x74\x2e\x62\x70\x66\x2e\x63\0\x5f\x5f\x73\x33\x32\x20\x42\x50\
\x46\x5f\x50\x52\x4f\x47\x28\x78\x6d\x5f\x74\x70\x5f\x62\x74\x66\x5f\x5f\x62\
\x6c\x6f\x63\x6b\x5f\x72\x71\x5f\x69\x6e\x73\x65\x72\x74\x2c\x20\x73\x74\x72\
\x75\x63\x74\x20\x72\x65\x71\x75\x65\x73\x74\x5f\x71\x75\x65\x75\x65\x20\x2a\
\x71\x2c\0\x78\x6d\x5f\x74\x70\x5f\x62\x74\x66\x5f\x5f\x62\x6c\x6f\x63\x6b\x5f\
\x72\x71\x5f\x69\x73\x73\x75\x65\0\x74\x70\x5f\x62\x74\x66\x2f\x62\x6c\x6f\x63\
\x6b\x5f\x72\x71\x5f\x69\x73\x73\x75\x65\0\x5f\x5f\x73\x33\x32\x20\x42\x50\x46\
\x5f\x50\x52\x4f\x47\x28\x78\x6d\x5f\x74\x70\x5f\x62\x74\x66\x5f\x5f\x62\x6c\
\x6f\x63\x6b\x5f\x72\x71\x5f\x69\x73\x73\x75\x65\x2c\x20\x73\x74\x72\x75\x63\
\x74\x20\x72\x65\x71\x75\x65\x73\x74\x5f\x71\x75\x65\x75\x65\x20\x2a\x71\x2c\0\
\x78\x6d\x5f\x74\x70\x5f\x62\x74\x66\x5f\x5f\x62\x6c\x6f\x63\x6b\x5f\x72\x71\
\x5f\x63\x6f\x6d\x70\x6c\x65\x74\x65\0\x74\x70\x5f\x62\x74\x66\x2f\x62\x6c\x6f\
\x63\x6b\x5f\x72\x71\x5f\x63\x6f\x6d\x70\x6c\x65\x74\x65\0\x5f\x5f\x73\x33\x32\
\x20\x42\x50\x46\x5f\x50\x52\x4f\x47\x28\x78\x6d\x5f\x74\x70\x5f\x62\x74\x66\
\x5f\x5f\x62\x6c\x6f\x63\x6b\x5f\x72\x71\x5f\x63\x6f\x6d\x70\x6c\x65\x74\x65\
\x2c\x20\x73\x74\x72\x75\x63\x74\x20\x72\x65\x71\x75\x65\x73\x74\x20\x2a\x72\
\x71\x2c\x20\x5f\x5f\x73\x33\x32\x20\x65\x72\x72\x6f\x72\x2c\0\x63\x68\x61\x72\
\0\x5f\x5f\x41\x52\x52\x41\x59\x5f\x53\x49\x5a\x45\x5f\x54\x59\x50\x45\x5f\x5f\
\0\x4c\x49\x43\x45\x4e\x53\x45\0\x6c\x69\x63\x65\x6e\x73\x65\0\0\x9f\xeb\x01\0\
\x20\0\0\0\0\0\0\0\x34\0\0\0\x34\0\0\0\x4c\0\0\0\x80\0\0\0\0\0\0\0\x08\0\0\0\
\x3d\0\0\0\x01\0\0\0\0\0\0\0\x06\0\0\0\xf1\0\0\0\x01\0\0\0\0\0\0\0\x08\0\0\0\
\x67\x01\0\0\x01\0\0\0\0\0\0\0\x0a\0\0\0\x10\0\0\0\x3d\0\0\0\x01\0\0\0\0\0\0\0\
\x54\0\0\0\x93\0\0\0\x07\x30\0\0\xf1\0\0\0\x01\0\0\0\0\0\0\0\x54\0\0\0\x07\x01\
\0\0\x07\x48\0\0\x67\x01\0\0\x01\0\0\0\0\0\0\0\x54\0\0\0\x80\x01\0\0\x07\x60\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\x03\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\x03\0\x05\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x14\0\0\0\x12\0\x03\0\0\0\0\
\0\0\0\0\0\x10\0\0\0\0\0\0\0\x54\0\0\0\x12\0\x04\0\0\0\0\0\0\0\0\0\x10\0\0\0\0\
\0\0\0\x84\0\0\0\x12\0\x05\0\0\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\xdb\0\0\0\x11\0\
\x06\0\0\0\0\0\0\0\0\0\x0d\0\0\0\0\0\0\0\x08\x01\0\0\0\0\0\0\x04\0\0\0\x07\0\0\
\0\x2c\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\x3c\0\0\0\0\0\0\0\x04\0\0\0\x02\0\0\0\
\x4c\0\0\0\0\0\0\0\x04\0\0\0\x03\0\0\0\x60\0\0\0\0\0\0\0\x04\0\0\0\x01\0\0\0\
\x78\0\0\0\0\0\0\0\x04\0\0\0\x02\0\0\0\x90\0\0\0\0\0\0\0\x04\0\0\0\x03\0\0\0\
\x0a\x0b\x0c\x0d\0\x2e\x74\x65\x78\x74\0\x2e\x72\x65\x6c\x2e\x42\x54\x46\x2e\
\x65\x78\x74\0\x78\x6d\x5f\x74\x70\x5f\x62\x74\x66\x5f\x5f\x62\x6c\x6f\x63\x6b\
\x5f\x72\x71\x5f\x69\x6e\x73\x65\x72\x74\0\x74\x70\x5f\x62\x74\x66\x2f\x62\x6c\
\x6f\x63\x6b\x5f\x72\x71\x5f\x69\x6e\x73\x65\x72\x74\0\x2e\x6c\x6c\x76\x6d\x5f\
\x61\x64\x64\x72\x73\x69\x67\0\x78\x6d\x5f\x74\x70\x5f\x62\x74\x66\x5f\x5f\x62\
\x6c\x6f\x63\x6b\x5f\x72\x71\x5f\x69\x73\x73\x75\x65\0\x74\x70\x5f\x62\x74\x66\
\x2f\x62\x6c\x6f\x63\x6b\x5f\x72\x71\x5f\x69\x73\x73\x75\x65\0\x78\x6d\x5f\x74\
\x70\x5f\x62\x74\x66\x5f\x5f\x62\x6c\x6f\x63\x6b\x5f\x72\x71\x5f\x63\x6f\x6d\
\x70\x6c\x65\x74\x65\0\x74\x70\x5f\x62\x74\x66\x2f\x62\x6c\x6f\x63\x6b\x5f\x72\
\x71\x5f\x63\x6f\x6d\x70\x6c\x65\x74\x65\0\x6c\x69\x63\x65\x6e\x73\x65\0\x2e\
\x73\x74\x72\x74\x61\x62\0\x2e\x73\x79\x6d\x74\x61\x62\0\x2e\x72\x65\x6c\x2e\
\x42\x54\x46\0\x4c\x49\x43\x45\x4e\x53\x45\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\xc2\0\0\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x5c\x05\
\0\0\0\0\0\0\xe3\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\x01\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x2f\0\0\0\x01\0\
\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x6e\0\0\0\x01\0\0\0\x06\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x50\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xa1\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\x60\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x08\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\xba\0\0\0\x01\0\0\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x70\0\0\0\
\0\0\0\0\x0d\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\xd6\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x80\0\0\0\0\0\0\0\x07\x03\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xd2\0\0\0\x09\0\
\0\0\x40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xe8\x04\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\
\x0c\0\0\0\x07\0\0\0\x08\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x0b\0\0\0\x01\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x88\x03\0\0\0\0\0\0\xa0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x07\0\0\0\x09\0\0\0\x40\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\xf8\x04\0\0\0\0\0\0\x60\0\0\0\0\0\0\0\x0c\0\0\0\x09\0\0\0\x08\
\0\0\0\0\0\0\0\x10\0\0\0\0\0\0\0\x46\0\0\0\x03\x4c\xff\x6f\0\0\0\x80\0\0\0\0\0\
\0\0\0\0\0\0\0\x58\x05\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\xca\0\0\0\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\x28\x04\0\0\0\0\0\0\xc0\0\0\0\0\0\0\0\x01\0\0\0\x04\0\0\0\x08\0\0\0\0\0\0\0\
\x18\0\0\0\0\0\0\0";

	return 0;
err:
	bpf_object__destroy_skeleton(s);
	return -ENOMEM;
}

#endif /* __XM_BIOLAT_BPF_SKEL_H__ */