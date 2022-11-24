/*
 * @Author: CALM.WU
 * @Date: 2022-11-17 11:53:49
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-11-18 15:29:43
 */

#pragma once

#include "../sock_info.h"

struct cds_lfht;
struct xm_mempool_s;
struct proc_file;

struct sock_info_node {
    struct sock_info si;

    // for rcu hash table
    struct cds_lfht_node hash_node;
    struct rcu_head      rcu_node;

    atomic_t is_update;
};

struct proc_sock_info_mgr {
    struct cds_lfht     *sock_info_node_rcu_ht;
    struct xm_mempool_s *sock_info_node_xmp;

    // pthread_spinlock_t spin_lock;   // for rcu write-side but only one thread write, so no need

    struct proc_file *proc_net_tcp;
    struct proc_file *proc_net_tcp6;
    struct proc_file *proc_net_udp;
    struct proc_file *proc_net_udp6;
    struct proc_file *proc_net_unix;
};

extern struct proc_sock_info_mgr *g_proc_sock_info_mgr;

extern int32_t init_sock_info_mgr();
extern void    fini_sock_info_mgr();

extern struct sock_info_node *alloc_sock_info_node();
extern int32_t                add_sock_info_node(struct sock_info_node *sin);
extern int32_t                find_sock_info_i(uint32_t ino, struct sock_info *res);
extern uint32_t               sock_info_count_i();
extern void                   clean_all_sock_info_update_flag();
extern void                   remove_all_not_update_sock_info();

extern void collect_socks_info_i();
