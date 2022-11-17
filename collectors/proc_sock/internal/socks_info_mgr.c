/*
 * @Author: CALM.WU
 * @Date: 2022-11-17 14:11:23
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-17 16:58:27
 */

#include "socks_internal.h"

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/mempool.h"
#include "utils/hash/xxhash.h"

static struct cds_lfht *__sock_info_rcu_ht = NULL;
struct xm_mempool_s    *__sock_info_xmp = NULL;

/**
 * It initializes the hash table
 *
 * @return The return value is the result of the function call.
 */
int32_t init_sock_info_mgr() {
    RUN_ONCE({
        // 构造rcu hash table
        __sock_info_rcu_ht = cds_lfht_new_flavor(
            8, 8, 0, CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING, &urcu_memb_flavor, NULL);
        if (unlikely(!__sock_info_rcu_ht)) {
            error("[PROC_SOCK] allocating hash table failed");
            return -1;
        }

        __sock_info_xmp = xm_mempool_init(sizeof(struct sock_info), 1024, 1024);
    });
    debug("[PROC_SOCK] init sock info mgr successed.");
    return 0;
}

struct sock_info *alloc_sock_info() {
    struct sock_info *si = (struct sock_info *)xm_mempool_malloc(__sock_info_xmp);
    return si;
}

static void __free_sock_info(struct rcu_head *rcu) {
    struct sock_info *si = container_of(rcu, struct sock_info, rcu);
    xm_mempool_free(__sock_info_xmp, si);
}

int32_t add_sock_info(struct sock_info *sock_info) {
    return 0;
}

static int32_t __match_sock_info(struct cds_lfht_node *node, const void *key) {
    return 0;
}

struct sock_info *find_sock_info(uint32_t ino) {
    struct sock_info     *si = NULL;
    struct cds_lfht_iter  iter; /* For iteration on hash table */
    struct cds_lfht_node *ht_node;

    // 计算sock inode hash值
    uint32_t hash = XXH32(&ino, sizeof(ino), (uint32_t)time(NULL));

    urcu_memb_read_lock();

    cds_lfht_lookup(__sock_info_rcu_ht, hash, __match_sock_info, &ino, &iter);
    ht_node = cds_lfht_iter_get_node(&iter);
    if (likely(ht_node)) {
        si = container_of(ht_node, struct sock_info, node);
        debug("[PROC_SOCK] find sock info successed. ino: %u, sock_type:%d", si->ino,
              si->sock_type);
    }
    urcu_memb_read_unlock();

    return si;
}

/**
 * It iterates over all the entries in the hash table, deletes them, and then calls the RCU callback
 * function to free the memory
 */
static void __del_all_sock_info() {
    struct cds_lfht_iter  iter;
    struct sock_info     *si = NULL;
    struct cds_lfht_node *ht_node;
    int32_t               ret = 0;

    debug("[PROC_SOCK] del all socks info");

    urcu_memb_read_lock();

    cds_lfht_for_each_entry(__sock_info_rcu_ht, &iter, si, node) {
        ht_node = cds_lfht_iter_get_node(&iter);
        ret = cds_lfht_del(__sock_info_rcu_ht, ht_node);
        if (!ret) {
            urcu_memb_call_rcu(&si->rcu, __free_sock_info);
        }
    }

    urcu_memb_read_unlock();
}

void del_all_sock_info() {
    __del_all_sock_info();
}

/**
 * It deletes all the socket info objects from the hash table, destroys the hash table, and destroys
 * the memory pool
 */
void fini_sock_info_mgr() {

    if (likely(__sock_info_rcu_ht)) {
        __del_all_sock_info();
        cds_lfht_destroy(__sock_info_rcu_ht, NULL);
    }

    if (likely(__sock_info_xmp)) {
        xm_mempool_fini(__sock_info_xmp);
    }
}
