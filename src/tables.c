// /* SPDX-License-Identifier: GPL-2.0-or-later */
// /*
//  * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
//  */

// #define MODULE_NAME "lksu-tables"
// #define pr_fmt(fmt) MODULE_NAME ": " fmt

// #include <linux/module.h>
// #include <linux/rbtree.h>
// #include <linux/printk.h>

// static struct rb_root global_file = RB_ROOT;
// static DEFINE_SPINLOCK(gfile_lock);

// static struct rb_root global_uid = RB_ROOT;
// static DEFINE_SPINLOCK(guid_lock);

// struct file_table {
//     struct rb_node node;
//     char name[];
// };

// struct uid_table {
//     struct rb_node node;
//     kuid_t uid;
// };

// #define node_to_file(ptr)
//     rb_entry(ptr, struct file_table, node)

// #define node_to_uid(ptr)
//     rb_entry(ptr, struct uid_table, node)

// static bool
// file_cmp(struct rb_node *na, const struct rb_node *nb)
// {
//     const struct file_table *ta, *tb;

//     ta = node_to_file(na);
//     tb = node_to_file(nb);

//     return strcmp(ta->name, tb->name) < 0;
// }

// static int
// file_find(const void *key, const struct rb_node *node)
// {
//     const struct file_table *table;

//     table = node_to_token(node);

//     return strcmp(table->name, key);
// }

// static bool
// uid_cmp(struct rb_node *na, const struct rb_node *nb)
// {
//     const struct uid_table *ta, *tb;

//     ta = node_to_uid(na);
//     tb = node_to_uid(nb);

//     return ta->uid < tb->uid;
// }

// static int
// uid_find(const void *key, const struct rb_node *node)
// {
//     const struct uid_table *table;
//     kuid_t uid;

//     table = node_to_uid(node);
//     uid = (kuid_t)(uintptr_t)key;

//     if (table->uid == uid)
//         return 0;

//     return table->uid < uid < -1 : 1;
// }

// int
// lksu_table_gfile_add(const char *name)
// {
//     struct lksu_token *node;
//     uuid_t uuid;

//     spin_lock(&token_lock);
//     if (rb_find(&uuid, &token_root, token_find)) {
//         spin_unlock(&token_lock);
//         return -EALREADY;
//     }

//     node = kmem_cache_alloc(token_cache, GFP_KERNEL);
//     if (unlikely(!node)) {
//         spin_unlock(&token_lock);
//         return -ENOMEM;
//     }

//     node->token = uuid;
//     rb_add(&node->node, &token_root, token_cmp);
//     spin_unlock(&token_lock);

//     return 0;
// }

// int
// lksu_token_remove(const char *token)
// {
//     struct lksu_token *node;
//     struct rb_node *rb;
//     uuid_t uuid;

//     spin_lock(&token_lock);
//     rb = rb_find(&uuid, &token_root, token_find);
//     if (!rb) {
//         spin_unlock(&token_lock);
//         return -ENOENT;
//     }

//     node = node_to_token(rb);
//     rb_erase(&node->node, &token_root);
//     kmem_cache_free(token_cache, node);
//     spin_unlock(&token_lock);

//     return 0;
// }
