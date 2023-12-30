/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#define MODULE_NAME "lksu-token"
#define pr_fmt(fmt) MODULE_NAME ": " fmt

#include "lksu.h"
#include "token.h"

#include <linux/module.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/printk.h>

static struct kmem_cache *token_cache __read_mostly;
static struct rb_root token_root = RB_ROOT;
static DEFINE_RWLOCK(token_lock);

struct lksu_token {
    struct rb_node node;
    uuid_t token;
};

#define node_to_token(ptr) \
    rb_entry(ptr, struct lksu_token, node)

static bool
token_cmp(struct rb_node *na, const struct rb_node *nb)
{
    const struct lksu_token *ta, *tb;

    ta = node_to_token(na);
    tb = node_to_token(nb);

    return memcmp(&ta->token, &tb->token, sizeof(guid_t)) < 0;
}

static int
token_find(const void *key, const struct rb_node *node)
{
    const struct lksu_token *token;
    const uuid_t *uuid;

    token = node_to_token(node);
    uuid = key;

    return memcmp(&token->token, uuid, sizeof(guid_t));
}

bool
lksu_token_verify(const char *token)
{
    struct rb_node *rb;
    uuid_t uuid;

    if (!uuid_is_valid(token)) {
        pr_notice("verify: uuid format invalid\n");
        return false;
    }
    uuid_parse(token, &uuid);

    read_lock(&token_lock);
    if (RB_EMPTY_ROOT(&token_root)) {
        read_unlock(&token_lock);
        return true;
    }

    rb = rb_find(&uuid, &token_root, token_find);
    read_unlock(&token_lock);

    return !!rb;
}

int
lksu_token_add(const char *token)
{
    struct lksu_token *node;
    uuid_t uuid;

    if (!uuid_is_valid(token)) {
        pr_notice("add: uuid format invalid\n");
        return -EINVAL;
    }
    uuid_parse(token, &uuid);

    write_lock(&token_lock);
    if (rb_find(&uuid, &token_root, token_find)) {
        write_unlock(&token_lock);
        return -EALREADY;
    }

    node = kmem_cache_alloc(token_cache, GFP_KERNEL);
    if (unlikely(!node)) {
        write_unlock(&token_lock);
        return -ENOMEM;
    }

    node->token = uuid;
    rb_add(&node->node, &token_root, token_cmp);
    write_unlock(&token_lock);

    return 0;
}

int
lksu_token_remove(const char *token)
{
    struct lksu_token *node;
    struct rb_node *rb;
    uuid_t uuid;

    if (!uuid_is_valid(token)) {
        pr_notice("remove: uuid format invalid\n");
        return -EINVAL;
    }
    uuid_parse(token, &uuid);

    write_lock(&token_lock);
    rb = rb_find(&uuid, &token_root, token_find);
    if (!rb) {
        write_unlock(&token_lock);
        return -ENOENT;
    }

    node = node_to_token(rb);
    rb_erase(&node->node, &token_root);
    kmem_cache_free(token_cache, node);
    write_unlock(&token_lock);

    return 0;
}

int __init
lksu_token_init(void)
{
    token_cache = KMEM_CACHE(lksu_token, 0);
    if (!token_cache)
        return -ENOMEM;

    rwlock_init(&token_lock);
    return 0;
}

void __exit
lksu_token_exit(void)
{
    kmem_cache_destroy(token_cache);
}
