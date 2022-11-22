/*
 * @Author: CALM.WU
 * @Date: 2022-11-22 16:04:56
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-22 16:29:57
 */

#pragma once

#include "compiler.h"
#include "list.h"

#define DEFINE_HASHTABLE(name, bits) \
    struct hlist_head name[1 << (bits)] = { [0 ...((1 << (bits)) - 1)] = HLIST_HEAD_INIT }

#define DEFINE_READ_MOSTLY_HASHTABLE(name, bits)                                       \
    struct hlist_head name[1 << (bits)] __read_mostly = { [0 ...((1 << (bits)) - 1)] = \
                                                              HLIST_HEAD_INIT }

#define DECLARE_HASHTABLE(name, bits) struct hlist_head name[1 << (bits)]

#define HASH_SIZE(name) (ARRAY_SIZE(name))
#define HASH_BITS(name) log2(HASH_SIZE(name))

#define GOLDEN_RATIO_32 0x61C88647
#define GOLDEN_RATIO_64 0x61C8864680B583EBull

static __always_inline uint32_t hash_32(uint32_t val, uint32_t bits) {
    /* High bits are more random, so use them. */
    return (val * GOLDEN_RATIO_32) >> (32 - bits);
}

static __always_inline uint32_t hash_64(uint64_t val, uint32_t bits) {
    return val * GOLDEN_RATIO_64 >> (64 - bits);
}

#define hash_min(val, bits) (sizeof(val) <= 4 ? hash_32(val, bits) : hash_64(val, bits))

static inline void __hash_init(struct hlist_head *ht, uint32_t sz) {
    uint32_t i;

    for (i = 0; i < sz; i++)
        INIT_HLIST_HEAD(&ht[i]);
}

/**
 * hash_init - initialize a hash table
 * @hashtable: hashtable to be initialized
 *
 * Calculates the size of the hashtable from the given parameter, otherwise
 * same as hash_init_size.
 *
 * This has to be a macro since HASH_BITS() will not work on pointers since
 * it calculates the size during preprocessing.
 */
#define hash_init(hashtable) __hash_init(hashtable, HASH_SIZE(hashtable))

/**
 * hash_add - add an object to a hashtable
 * @hashtable: hashtable to add to
 * @node: the &struct hlist_node of the object to be added
 * @key: the key of the object to be added
 */
#define hash_add(hashtable, node, key) \
    hlist_add_head(node, &hashtable[hash_min(key, HASH_BITS(hashtable))])

/**
 * hash_hashed - check whether an object is in any hashtable
 * @node: the &struct hlist_node of the object to be checked
 */
static inline bool hash_hashed(struct hlist_node *node) {
    return !hlist_unhashed(node);
}

static inline bool __hash_empty(struct hlist_head *ht, unsigned int sz) {
    unsigned int i;

    for (i = 0; i < sz; i++)
        if (!hlist_empty(&ht[i]))
            return false;

    return true;
}

/**
 * hash_empty - check whether a hashtable is empty
 * @hashtable: hashtable to check
 *
 * This has to be a macro since HASH_BITS() will not work on pointers since
 * it calculates the size during preprocessing.
 */
#define hash_empty(hashtable) __hash_empty(hashtable, HASH_SIZE(hashtable))

/**
 * hash_del - remove an object from a hashtable
 * @node: &struct hlist_node of the object to remove
 */
static inline void hash_del(struct hlist_node *node) {
    hlist_del_init(node);
}

/**
 * hash_for_each - iterate over a hashtable
 * @name: hashtable to iterate
 * @bkt: integer to use as bucket loop cursor
 * @obj: the type * to use as a loop cursor for each entry
 * @member: the name of the hlist_node within the struct
 */
#define hash_for_each(name, bkt, obj, member)                                    \
    for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < HASH_SIZE(name); (bkt)++) \
    hlist_for_each_entry(obj, &name[bkt], member)

/**
 * hash_for_each_safe - iterate over a hashtable safe against removal of
 * hash entry
 * @name: hashtable to iterate
 * @bkt: integer to use as bucket loop cursor
 * @tmp: a &struct hlist_node used for temporary storage
 * @obj: the type * to use as a loop cursor for each entry
 * @member: the name of the hlist_node within the struct
 */
#define hash_for_each_safe(name, bkt, tmp, obj, member)                          \
    for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < HASH_SIZE(name); (bkt)++) \
    hlist_for_each_entry_safe(obj, tmp, &name[bkt], member)

/**
 * hash_for_each_possible - iterate over all possible objects hashing to the
 * same bucket
 * @name: hashtable to iterate
 * @obj: the type * to use as a loop cursor for each entry
 * @member: the name of the hlist_node within the struct
 * @key: the key of the objects to iterate over
 */
#define hash_for_each_possible(name, obj, member, key) \
    hlist_for_each_entry(obj, &name[hash_min(key, HASH_BITS(name))], member)

/**
 * hash_for_each_possible_safe - iterate over all possible objects hashing to the
 * same bucket safe against removals
 * @name: hashtable to iterate
 * @obj: the type * to use as a loop cursor for each entry
 * @tmp: a &struct hlist_node used for temporary storage
 * @member: the name of the hlist_node within the struct
 * @key: the key of the objects to iterate over
 */
#define hash_for_each_possible_safe(name, obj, tmp, member, key) \
    hlist_for_each_entry_safe(obj, tmp, &name[hash_min(key, HASH_BITS(name))], member)
