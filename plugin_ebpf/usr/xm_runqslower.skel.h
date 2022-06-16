/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */

/* THIS FILE IS AUTOGENERATED! */
#ifndef __XM_RUNQSLOWER_BPF_SKEL_H__
#define __XM_RUNQSLOWER_BPF_SKEL_H__

#include <errno.h>
#include <stdlib.h>
#include <bpf/libbpf.h>

struct xm_runqslower_bpf {
	struct bpf_object_skeleton *skeleton;
	struct bpf_object *obj;
};

static void
xm_runqslower_bpf__destroy(struct xm_runqslower_bpf *obj)
{
	if (!obj)
		return;
	if (obj->skeleton)
		bpf_object__destroy_skeleton(obj->skeleton);
	free(obj);
}

static inline int
xm_runqslower_bpf__create_skeleton(struct xm_runqslower_bpf *obj);

static inline struct xm_runqslower_bpf *
xm_runqslower_bpf__open_opts(const struct bpf_object_open_opts *opts)
{
	struct xm_runqslower_bpf *obj;
	int err;

	obj = (struct xm_runqslower_bpf *)calloc(1, sizeof(*obj));
	if (!obj) {
		errno = ENOMEM;
		return NULL;
	}

	err = xm_runqslower_bpf__create_skeleton(obj);
	if (err)
		goto err_out;

	err = bpf_object__open_skeleton(obj->skeleton, opts);
	if (err)
		goto err_out;

	return obj;
err_out:
	xm_runqslower_bpf__destroy(obj);
	errno = -err;
	return NULL;
}

static inline struct xm_runqslower_bpf *
xm_runqslower_bpf__open(void)
{
	return xm_runqslower_bpf__open_opts(NULL);
}

static inline int
xm_runqslower_bpf__load(struct xm_runqslower_bpf *obj)
{
	return bpf_object__load_skeleton(obj->skeleton);
}

static inline struct xm_runqslower_bpf *
xm_runqslower_bpf__open_and_load(void)
{
	struct xm_runqslower_bpf *obj;
	int err;

	obj = xm_runqslower_bpf__open();
	if (!obj)
		return NULL;
	err = xm_runqslower_bpf__load(obj);
	if (err) {
		xm_runqslower_bpf__destroy(obj);
		errno = -err;
		return NULL;
	}
	return obj;
}

static inline int
xm_runqslower_bpf__attach(struct xm_runqslower_bpf *obj)
{
	return bpf_object__attach_skeleton(obj->skeleton);
}

static inline void
xm_runqslower_bpf__detach(struct xm_runqslower_bpf *obj)
{
	return bpf_object__detach_skeleton(obj->skeleton);
}

static inline const void *xm_runqslower_bpf__elf_bytes(size_t *sz);

static inline int
xm_runqslower_bpf__create_skeleton(struct xm_runqslower_bpf *obj)
{
	struct bpf_object_skeleton *s;

	s = (struct bpf_object_skeleton *)calloc(1, sizeof(*s));
	if (!s)
		goto err;

	s->sz = sizeof(*s);
	s->name = "xm_runqslower_bpf";
	s->obj = &obj->obj;

	s->data = (void *)xm_runqslower_bpf__elf_bytes(&s->data_sz);

	obj->skeleton = s;
	return 0;
err:
	bpf_object__destroy_skeleton(s);
	return -ENOMEM;
}

static inline const void *xm_runqslower_bpf__elf_bytes(size_t *sz)
{
	*sz = 448;
	return (const void *)"\
\x7f\x45\x4c\x46\x02\x01\x01\0\0\0\0\0\0\0\0\0\x01\0\xf7\0\x01\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\x80\0\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\x40\0\x05\0\x03\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x2e\x74\x65\x78\x74\0\x2e\
\x6c\x6c\x76\x6d\x5f\x61\x64\x64\x72\x73\x69\x67\0\x2e\x73\x74\x72\x74\x61\x62\
\0\x2e\x73\x79\x6d\x74\x61\x62\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\0\0\x01\0\0\0\x01\0\0\0\x06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x1d\0\0\
\0\x02\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x40\0\0\0\0\0\0\0\x18\0\0\0\0\0\0\
\0\x03\0\0\0\x01\0\0\0\x08\0\0\0\0\0\0\0\x18\0\0\0\0\0\0\0\x15\0\0\0\x03\0\0\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x58\0\0\0\0\0\0\0\x25\0\0\0\0\0\0\0\0\0\0\0\0\
\0\0\0\x01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x07\0\0\0\x03\x4c\xff\x6f\0\0\0\x80\0\
\0\0\0\0\0\0\0\0\0\0\0\x7d\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01\0\
\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
}

#endif /* __XM_RUNQSLOWER_BPF_SKEL_H__ */
