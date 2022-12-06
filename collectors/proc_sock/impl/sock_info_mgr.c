/*
 * @Author: CALM.WU
 * @Date: 2022-11-17 14:11:23
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-18 16:35:04
 */

// look vsock.c

#include "sock_info_mgr.h"

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/mempool.h"
#include "utils/hash/xxhash.h"
#include "utils/procfile.h"

struct proc_sock_info_mgr *g_proc_sock_info_mgr = NULL;

static uint32_t __rand_seed = 0;

/**
 * It initializes a global hash table and a global memory pool
 */
int32_t init_sock_info_mgr() {
    RUN_ONCE({
        __rand_seed = (uint32_t)time(NULL);

        g_proc_sock_info_mgr =
            (struct proc_sock_info_mgr *)calloc(1, sizeof(struct proc_sock_info_mgr));
        if (unlikely(!g_proc_sock_info_mgr)) {
            error("[PROC_SOCK] calloc sock_info_node manager failed.");
            return -1;
        }

        // 构造rcu hash table
        g_proc_sock_info_mgr->sock_info_node_rcu_ht = cds_lfht_new_flavor(
            8, 8, 0, CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING, &urcu_memb_flavor, NULL);
        if (unlikely(!g_proc_sock_info_mgr->sock_info_node_rcu_ht)) {
            error("[PROC_SOCK] allocating hash table failed");
            goto FAILED;
        }

        g_proc_sock_info_mgr->sock_info_node_xmp =
            xm_mempool_init(sizeof(struct sock_info_node), 1024, 1024);
        if (unlikely(!g_proc_sock_info_mgr->sock_info_node_xmp)) {
            error("[PROC_SOCK] create sock_info_node memory pool failed");
            goto FAILED;
        }

        // pthread_spin_init(&g_proc_sock_info_mgr->spin_lock, PTHREAD_PROCESS_PRIVATE);
    });

    debug("[PROC_SOCK] sock_info_node manager init successed.");
    return 0;

FAILED:
    if (g_proc_sock_info_mgr->sock_info_node_rcu_ht) {
        cds_lfht_destroy(g_proc_sock_info_mgr->sock_info_node_rcu_ht, NULL);
    }

    if (g_proc_sock_info_mgr) {
        free(g_proc_sock_info_mgr);
    }

    // pthread_spin_destroy(&g_proc_sock_info_mgr->spin_lock);
    return -1;
}

/**
 * It allocates a new sock_info_node structure from the memory pool
 *
 * @return A pointer to a struct sock_info_node
 */
struct sock_info_node *alloc_sock_info_node() {
    struct sock_info_node *sin =
        (struct sock_info_node *)xm_mempool_malloc(g_proc_sock_info_mgr->sock_info_node_xmp);
    return sin;
}

/**
 * It frees the memory allocated for the socket information structure
 *
 * @param rcu_node the rcu_node head
 */
static void __free_sock_info_node(struct rcu_head *rcu_node) {
    struct sock_info_node *sin = container_of(rcu_node, struct sock_info_node, rcu_node);
    debug("[PROC_SOCK] free sock_info_node ino:%u", sin->si.ino);
    xm_mempool_free(g_proc_sock_info_mgr->sock_info_node_xmp, sin);
}

/**
 * It returns 0 if the inode number of the socket info structure pointed to by the node parameter is
 * equal to the inode number pointed to by the key parameter
 *
 * @param node the node to match
 * @param key the key to search for
 *
 * @return The inode number of the socket.
 */
static int32_t __match_sock_info(struct cds_lfht_node *ht_node, const void *key) {
    const uint32_t        *p_ino = (const uint32_t *)key;
    struct sock_info_node *sin = container_of(ht_node, struct sock_info_node, hash_node);
    return (*p_ino) == sin->si.ino;
}

/**
 * It adds a new node to a hash table
 *
 * @param new_sin the new sock_info_node to be added
 */
int32_t add_sock_info_node(struct sock_info_node *new_sin) {
    struct cds_lfht_node  *ht_node;
    struct cds_lfht_iter   iter;
    struct sock_info_node *old_sin;
    bool                   use_new = false;

    if (likely(new_sin)) {
        cds_lfht_node_init(&new_sin->hash_node);

        uint32_t hash = XXH32(&new_sin->si.ino, sizeof(uint32_t), __rand_seed);

        urcu_memb_read_lock();

        // 查找ino是否已经存在，而且类型是否相同
        cds_lfht_lookup(g_proc_sock_info_mgr->sock_info_node_rcu_ht, hash, __match_sock_info,
                        &new_sin->si.ino, &iter);
        ht_node = cds_lfht_iter_get_node(&iter);
        if (likely(ht_node)) {
            old_sin = container_of(ht_node, struct sock_info_node, hash_node);

            // 如果类型不同，替换
            // pthread_spin_lock(&g_proc_sock_info_mgr->spin_lock);
            if (old_sin->si.sock_type != new_sin->si.sock_type
                || old_sin->si.sock_state != new_sin->si.sock_state) {
                use_new = true;
            } else {
                atomic_inc(&old_sin->is_update);
                xm_mempool_free(g_proc_sock_info_mgr->sock_info_node_xmp, new_sin);
            }
        } else {
            use_new = true;
        }

        if (use_new) {
            // 替换或者添加
            ht_node =
                cds_lfht_add_replace(g_proc_sock_info_mgr->sock_info_node_rcu_ht, hash,
                                     __match_sock_info, &new_sin->si.ino, &new_sin->hash_node);
            if (ht_node) {
                debug("[PROC_SOCK] replace old sock_info_node ino:%u, type:%d, state:%d ===> new "
                      "sock_info_node ino:%d, type:%d, state:%d",
                      old_sin->si.ino, old_sin->si.sock_type, old_sin->si.sock_state,
                      new_sin->si.ino, new_sin->si.sock_type, new_sin->si.sock_state);

                old_sin = container_of(ht_node, struct sock_info_node, hash_node);
                urcu_memb_call_rcu(&old_sin->rcu_node, __free_sock_info_node);
            } else {
                debug("[PROC_SOCK] add sock_info_node ino:%u, type:%d, is_update:%d, hash:%u",
                      new_sin->si.ino, new_sin->si.sock_type, atomic_read(&new_sin->is_update),
                      hash);
            }
        }

        urcu_memb_read_unlock();
    }
    return 0;
}

int32_t find_sock_info_i(uint32_t ino, struct sock_info *res) {
    struct sock_info_node *sin = NULL;
    struct cds_lfht_iter   iter; /* For iteration on hash table */
    struct cds_lfht_node  *ht_node;
    int32_t                ret = 0;

    if (unlikely(!res)) {
        return -1;
    }

    // 计算sock inode hash值
    uint32_t hash = XXH32(&ino, sizeof(ino), __rand_seed);

    urcu_memb_read_lock();
    // 在hash table中查找，first use hash value，second use key
    cds_lfht_lookup(g_proc_sock_info_mgr->sock_info_node_rcu_ht, hash, __match_sock_info, &ino,
                    &iter);
    ht_node = cds_lfht_iter_get_node(&iter);
    if (likely(ht_node)) {
        sin = container_of(ht_node, struct sock_info_node, hash_node);
        __builtin_memcpy(res, &sin->si, sizeof(struct sock_info));
        ret = 0;
        debug("[PROC_SOCK] find sock_info_node successed. ino: %u, sock_type:%d, sock_stat:%d",
              res->ino, res->sock_type, res->sock_state);
    } else {
        debug("[PROC_SOCK] sock_info_node ino: %u not found", ino);
        ret = -1;
    }

    urcu_memb_read_unlock();

    return ret;
}

/**
 * "For each entry in the hash table, increment the count."
 *
 * @return The number of elements in the hash table.
 */
uint32_t sock_info_count_i() {
    struct sock_info_node *sin = NULL;
    struct cds_lfht_iter   iter; /* For iteration on hash table */
    uint32_t               count = 0;

    urcu_memb_read_lock();
    cds_lfht_for_each_entry(g_proc_sock_info_mgr->sock_info_node_rcu_ht, &iter, sin, hash_node) {
        count++;
    }
    urcu_memb_read_unlock();
    return count;
}

/**
 * It iterates over all the entries in the hash table, deletes them, and then calls the RCU callback
 * function to free the memory
 */
static void __remove_all_sock_info() {
    struct cds_lfht_iter   iter;
    struct cds_lfht_node  *ht_node;
    struct sock_info_node *sin = NULL;
    int32_t                ret = 0;

    debug("[PROC_SOCK] remove all sock_info_node");
    urcu_memb_read_lock();
    cds_lfht_for_each_entry(g_proc_sock_info_mgr->sock_info_node_rcu_ht, &iter, sin, hash_node) {
        ht_node = cds_lfht_iter_get_node(&iter);
        ret = cds_lfht_del(g_proc_sock_info_mgr->sock_info_node_rcu_ht, ht_node);
        if (!ret) {
            // debug("[PROC_SOCK] remove sock_info_node ino: %u, sock_type:%d", sin->si.ino,
            //       sin->si.sock_type);
            urcu_memb_call_rcu(&sin->rcu_node, __free_sock_info_node);
        }
    }
    urcu_memb_read_unlock();
    /*
     * Waiting for previously called call_rcu handlers to complete
     * before program exits, or in library destructors, is a good
     * practice.
     */
    urcu_memb_barrier();
}

/**
 * It iterates through a hash table, and sets a flag to 0 for each entry in the hash table
 */
void clean_all_sock_info_update_flag() {
    struct cds_lfht_iter   iter;
    struct cds_lfht_node  *ht_node;
    struct sock_info_node *sin;
    int32_t                ret = 0;

    debug("[PROC_SOCK] clean all sock_info_node update flag");

    urcu_memb_read_lock();
    cds_lfht_for_each_entry(g_proc_sock_info_mgr->sock_info_node_rcu_ht, &iter, sin, hash_node) {
        atomic_dec(&sin->is_update);
    }
    urcu_memb_read_unlock();
}

/**
 * It iterates over a hash table, and deletes all entries that have a certain flag set
 */
void remove_all_not_update_sock_info() {
    struct cds_lfht_iter   iter;
    struct cds_lfht_node  *ht_node;
    struct sock_info_node *sin;
    int32_t                ret = 0;

    debug("[PROC_SOCK] remove all not update sock_info_node");

    urcu_memb_read_lock();
    cds_lfht_for_each_entry(g_proc_sock_info_mgr->sock_info_node_rcu_ht, &iter, sin, hash_node) {
        ht_node = cds_lfht_iter_get_node(&iter);
        if (atomic_read(&sin->is_update) == 0) {
            debug("[PROC_SOCK] remove not update sock_info_node ino: %u, sock_type:%d", sin->si.ino,
                  sin->si.sock_type);
            ret = cds_lfht_del(g_proc_sock_info_mgr->sock_info_node_rcu_ht, ht_node);
            if (!ret) {
                urcu_memb_call_rcu(&sin->rcu_node, __free_sock_info_node);
            }
        }
    }
    urcu_memb_read_unlock();
}

/**
 * It deletes all the socket info objects from the hash table, destroys the hash table, and destroys
 * the memory pool
 */
void fini_sock_info_mgr() {

#define FREE_PROCFILE(pf)       \
    do {                        \
        if (likely(pf)) {       \
            procfile_close(pf); \
            pf = NULL;          \
        }                       \
    } while (0)

    if (likely(g_proc_sock_info_mgr)) {

        FREE_PROCFILE(g_proc_sock_info_mgr->proc_net_unix);
        FREE_PROCFILE(g_proc_sock_info_mgr->proc_net_tcp);
        FREE_PROCFILE(g_proc_sock_info_mgr->proc_net_tcp6);
        FREE_PROCFILE(g_proc_sock_info_mgr->proc_net_udp);
        FREE_PROCFILE(g_proc_sock_info_mgr->proc_net_udp6);

        if (likely(g_proc_sock_info_mgr->sock_info_node_rcu_ht)) {
            __remove_all_sock_info();
            cds_lfht_destroy(g_proc_sock_info_mgr->sock_info_node_rcu_ht, NULL);
        }

        if (likely(g_proc_sock_info_mgr->sock_info_node_xmp)) {
            xm_mempool_fini(g_proc_sock_info_mgr->sock_info_node_xmp);
        }

        free(g_proc_sock_info_mgr);
        g_proc_sock_info_mgr = NULL;

        debug("[PROC_SOCK] sock_info_node manager finalize.");
    }
}
