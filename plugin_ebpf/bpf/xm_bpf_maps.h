/*
 * @Author: CALM.WU
 * @Date: 2022-10-31 15:52:09
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-10-31 15:55:14
 */

#pragma once

#include <bpf/bpf_helpers.h>
#include <asm-generic/errno.h>

/**
 * Looks up a value in a map and initializes it if it doesn't exist.
 *
 * @param map The map to look up the value in.
 * @param key The key to look up the value with.
 * @param init_val The value to initialize the map with if it doesn't exist.
 *
 * @returns The value in the map, or the value initialized by init_val if the
 *          map didn't exist.
 */
static __always_inline void *bpf_map_lookup_or_try_init(void *map, const void *key,
                                                        const void *init_val) {
    void *val = bpf_map_lookup_elem(map, key);
    if (val) {
        return val;
    }

    __s32 err_no = bpf_map_update_elem(map, key, init_val, BPF_NOEXIST);
    if (err_no && err_no != -EEXIST) {
        return 0;
    }
    return bpf_map_lookup_elem(map, key);
}