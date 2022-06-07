/*
 * @Author: CALM.WU
 * @Date: 2021-11-19 10:42:05
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2021-11-19 11:01:05
 */

#pragma once

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int32_t str_split_to_nums(const char *str, const char *delim, uint64_t *nums,
                                 uint16_t nums_max_size);

extern int32_t            str2int32_t(const char *s);
extern uint32_t           str2uint32_t(const char *s);
extern uint64_t           str2uint64_t(const char *s);
extern unsigned long      str2ul(const char *s);
extern unsigned long long str2ull(const char *s);
extern int64_t            str2int64_t(const char *s, char **endptr);
extern long double        str2ld(const char *s, char **endptr);

extern char *itoa(int32_t value, char *result, int32_t base);

extern uint32_t bkrd_hash(const void *key, size_t len);

extern uint32_t simple_hash(const char *name);

static __always_inline char *strreplace(char *s, char old, char new) {
    for (; *s; ++s)
        if (*s == old)
            *s = new;
    return s;
}

extern size_t strlcpy(char *dst, const char *src, size_t siz);

extern char **strsplit(const char *s, const char *delim);

extern char **strsplit_count(const char *s, const char *delim, size_t *nb);

#define str_has_pfx(str, pfx) \
    (strncmp(str, pfx, __builtin_constant_p(pfx) ? sizeof(pfx) - 1 : strlen(pfx)) == 0)

#ifdef __cplusplus
}
#endif