/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) 1999  Andrea Arcangeli <andrea@suse.de>
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _LOCAL_RBTREE_H_
#define _LOCAL_RBTREE_H_

#include <linux/stddef.h>
#include <linux/rbtree.h>

static __always_inline void
lksu_rb_add(struct rb_node *node, struct rb_root *tree,
            bool (*less)(struct rb_node *, const struct rb_node *))
{
    struct rb_node **link = &tree->rb_node;
    struct rb_node *parent = NULL;

    while (*link) {
        parent = *link;
        if (less(node, parent))
            link = &parent->rb_left;
        else
            link = &parent->rb_right;
    }

    rb_link_node(node, parent, link);
    rb_insert_color(node, tree);
}

static __always_inline struct rb_node *
lksu_rb_find(const void *key, const struct rb_root *tree,
             int (*cmp)(const void *key, const struct rb_node *))
{
    struct rb_node *node = tree->rb_node;

    while (node) {
        int c = cmp(key, node);

        if (c < 0)
            node = node->rb_left;
        else if (c > 0)
            node = node->rb_right;
        else
            return node;
    }

    return NULL;
}

#endif /* _LOCAL_RBTREE_H_ */
