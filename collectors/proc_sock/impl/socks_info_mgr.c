/*
 * @Author: CALM.WU
 * @Date: 2022-11-17 14:11:23
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-18 16:35:04
 */

// look vsock.c

#include "socks_info_mgr.h"

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/mempool.h"
#include "utils/hash/xxhash.h"
#include "utils/procfile.h"

struct proc_sock_info_mgr *g_proc_sock_info_mgr = NULL;

/**
 * It initializes the hash table and memory pool for the socket information
 *
 * @return A pointer to a struct sock_info.
 */
int32_t init_sock_info_mgr() {
    RUN_ONCE({
        g_proc_sock_info_mgr =
            (struct proc_sock_info_mgr *)calloc(1, sizeof(struct proc_sock_info_mgr));
        if (unlikely(!g_proc_sock_info_mgr)) {
            error("[PROC_SOCK] calloc sock_info manager failed.");
            return -1;
        }

        // 构造rcu hash table
        g_proc_sock_info_mgr->sock_info_rcu_ht = cds_lfht_new_flavor(
            8, 8, 0, CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING, &urcu_memb_flavor, NULL);
        if (unlikely(!g_proc_sock_info_mgr->sock_info_rcu_ht)) {
            error("[PROC_SOCK] allocating hash table failed");
            goto FAILED;
        }

        g_proc_sock_info_mgr->sock_info_xmp = xm_mempool_init(sizeof(struct sock_info), 1024, 1024);
        if (unlikely(!g_proc_sock_info_mgr->sock_info_xmp)) {
            error("[PROC_SOCK] create sock_info memory pool failed");
            goto FAILED;
        }
    });
    debug("[PROC_SOCK] sock_info manager init successed.");
    return 0;

FAILED:
    if (g_proc_sock_info_mgr->sock_info_rcu_ht) {
        cds_lfht_destroy(g_proc_sock_info_mgr->sock_info_rcu_ht, NULL);
    }

    if (g_proc_sock_info_mgr) {
        free(g_proc_sock_info_mgr);
    }
    return -1;
}

/**
 * It allocates a new sock_info structure from the memory pool
 *
 * @return A pointer to a struct sock_info
 */
struct sock_info *alloc_sock_info() {
    struct sock_info *si =
        (struct sock_info *)xm_mempool_malloc(g_proc_sock_info_mgr->sock_info_xmp);
    return si;
}

/**
 * It frees the memory allocated for the socket information structure
 *
 * @param rcu the rcu head
 */
static void __free_sock_info(struct rcu_head *rcu) {
    struct sock_info *si = container_of(rcu, struct sock_info, rcu);
    debug("[PROC_SOCK] free sock_info ino:%u", si->ino);
    xm_mempool_free(g_proc_sock_info_mgr->sock_info_xmp, si);
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
    const uint32_t *  p_ino = (const uint32_t *)key;
    struct sock_info *si = container_of(ht_node, struct sock_info, node);
    return (*p_ino) == si->ino;
}

/**
 * It adds a new sock_info to the hash table
 *
 * @param sock_info the data to be added to the hash table
 *
 * @return the node.
 */
int32_t add_sock_info(struct sock_info *sock_info) {
    struct cds_lfht_node *ht_node;

    if (likely(sock_info)) {
        cds_lfht_node_init(&sock_info->node);

        uint32_t hash = XXH32(&sock_info->ino, sizeof(uint32_t), (uint32_t)time(NULL));

        urcu_memb_read_lock();

        // sock ino可能重复
        ht_node = cds_lfht_add_replace(g_proc_sock_info_mgr->sock_info_rcu_ht, hash,
                                       __match_sock_info, &sock_info->ino, &sock_info->node);
        if (ht_node) {
            // replace
            struct sock_info *old_si = container_of(ht_node, struct sock_info, node);
            debug("[PROC_SOCK] replace sock_info ino:%u", old_si->ino);
            urcu_memb_call_rcu(&old_si->rcu, __free_sock_info);
        } else {
            // add
            debug("[PROC_SOCK] add sock_info ino:%u", sock_info->ino);
        }

        urcu_memb_read_unlock();
    }
    return 0;
}

/**
 * It finds a sock_info struct in the hash table by inode number
 *
 * @param ino inode number of the socket
 *
 * @return A pointer to a sock_info struct.
 */
struct sock_info *find_sock_info_i(uint32_t ino) {
    struct sock_info *    si = NULL;
    struct cds_lfht_iter  iter; /* For iteration on hash table */
    struct cds_lfht_node *ht_node;

    // 计算sock inode hash值
    uint32_t hash = XXH32(&ino, sizeof(ino), (uint32_t)time(NULL));

    // 在hash table中查找，first use hash value，second use key
    cds_lfht_lookup(g_proc_sock_info_mgr->sock_info_rcu_ht, hash, __match_sock_info, &ino, &iter);
    ht_node = cds_lfht_iter_get_node(&iter);
    if (likely(ht_node)) {
        si = container_of(ht_node, struct sock_info, node);
        debug("[PROC_SOCK] find sock_info successed. ino: %u, sock_type:%d", si->ino,
              si->sock_type);
    }

    return si;
}

/**
 * It iterates over all the entries in the hash table, deletes them, and then calls the RCU callback
 * function to free the memory
 */
static void __remove_all_sock_info() {
    struct cds_lfht_iter  iter;
    struct sock_info *    si = NULL;
    struct cds_lfht_node *ht_node;
    int32_t               ret = 0;

    debug("[PROC_SOCK] remove all sock_info");

    urcu_memb_read_lock();

    cds_lfht_for_each_entry(g_proc_sock_info_mgr->sock_info_rcu_ht, &iter, si, node) {
        ht_node = cds_lfht_iter_get_node(&iter);
        ret = cds_lfht_del(g_proc_sock_info_mgr->sock_info_rcu_ht, ht_node);
        if (!ret) {
            urcu_memb_call_rcu(&si->rcu, __free_sock_info);
        }
    }

    urcu_memb_read_unlock();

    urcu_memb_synchronize_rcu();
}

/**
 * It iterates through a hash table, and sets a flag to 0 for each entry in the hash table
 */
void clean_all_sock_info_update_flag() {
    struct cds_lfht_iter  iter;
    struct cds_lfht_node *ht_node;
    struct sock_info *    si;
    int32_t               ret = 0;

    debug("[PROC_SOCK] clean all sock_info update flag");

    urcu_memb_read_lock();
    cds_lfht_for_each_entry(g_proc_sock_info_mgr->sock_info_rcu_ht, &iter, si, node) {
        atomic_dec(&si->is_update);
    }
    urcu_memb_read_unlock();
}

/**
 * It iterates over a hash table, and deletes all entries that have a certain flag set
 */
void remove_all_not_update_sock_info() {
    struct cds_lfht_iter  iter;
    struct cds_lfht_node *ht_node;
    struct sock_info *    si;
    int32_t               ret = 0;

    debug("[PROC_SOCK] remove all not update sock_info");

    urcu_memb_read_lock();

    cds_lfht_for_each_entry(g_proc_sock_info_mgr->sock_info_rcu_ht, &iter, si, node) {
        ht_node = cds_lfht_iter_get_node(&iter);
        if (atomic_read(si->is_update) == 0) {
            ret = cds_lfht_del(g_proc_sock_info_mgr->sock_info_rcu_ht, ht_node);
            if (!ret) {
                urcu_memb_call_rcu(&si->rcu, __free_sock_info);
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

        if (likely(g_proc_sock_info_mgr->sock_info_rcu_ht)) {
            __remove_all_sock_info();
            cds_lfht_destroy(g_proc_sock_info_mgr->sock_info_rcu_ht, NULL);
        }

        if (likely(g_proc_sock_info_mgr->sock_info_xmp)) {
            xm_mempool_fini(g_proc_sock_info_mgr->sock_info_xmp);
        }

        free(g_proc_sock_info_mgr);
        g_proc_sock_info_mgr = NULL;

        debug("[PROC_SOCK] sock_info manager finalize.");
    }
}
