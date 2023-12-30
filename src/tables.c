/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#define MODULE_NAME "lksu-tables"
#define pr_fmt(fmt) MODULE_NAME ": " fmt

#include "lksu.h"
#include "tables.h"

#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/rbtree.h>
#include <linux/printk.h>

static struct rb_root global_file = RB_ROOT;
static DEFINE_SPINLOCK(gfile_lock);

static struct rb_root global_uid = RB_ROOT;
static DEFINE_SPINLOCK(guid_lock);

struct dirent_state {
    const char *name;
    size_t length;
};

struct file_table {
    struct rb_node node;
    size_t dirlen;
    char name[];
};

struct uid_table {
    struct rb_node node;
    uid_t uid;
};

#define node_to_file(ptr) \
    rb_entry(ptr, struct file_table, node)

#define node_to_uid(ptr) \
    rb_entry(ptr, struct uid_table, node)

static bool
file_cmp(struct rb_node *na, const struct rb_node *nb)
{
    const struct file_table *ta, *tb;

    ta = node_to_file(na);
    tb = node_to_file(nb);

    return strcmp(ta->name, tb->name) < 0;
}

static int
file_find(const void *key, const struct rb_node *node)
{
    const struct file_table *table;

    table = node_to_file(node);

    return strcmp(table->name, key);
}

static int
dirent_find(const void *key, const struct rb_node *node)
{
    const struct file_table *table;
    const struct dirent_state *dirent;

    table = node_to_file(node);
    dirent = key;

    if (table->dirlen != dirent->length)
        return strcmp(table->name, dirent->name);

    return strncmp(table->name, dirent->name, dirent->length);
}

static bool
uid_cmp(struct rb_node *na, const struct rb_node *nb)
{
    const struct uid_table *ta, *tb;

    ta = node_to_uid(na);
    tb = node_to_uid(nb);

    return ta->uid < tb->uid;
}

static int
uid_find(const void *key, const struct rb_node *node)
{
    const struct uid_table *table;
    uid_t uid;

    table = node_to_uid(node);
    uid = (uid_t)(uintptr_t)key;

    if (table->uid == uid)
        return 0;

    return table->uid < uid ? -1 : 1;
}

bool
lksu_table_gfile_check(const char *name)
{
    struct rb_node *rb;

    spin_lock(&gfile_lock);
    rb = rb_find(name, &global_file, file_find);
    spin_unlock(&gfile_lock);

    return !!rb;
}

extern bool
lksu_table_gdirent_check(const char *name)
{
    struct dirent_state state;
    struct rb_node *rb;

    state.name = name;
    state.length = strlen(name);

    spin_lock(&gfile_lock);
    rb = rb_find(&state, &global_file, dirent_find);
    spin_unlock(&gfile_lock);

    return !!rb;
}

int
lksu_table_gfile_add(const char *name)
{
    struct file_table *node;
    const char *path;
    size_t length;

    if (!*name)
        return -EINVAL;

    spin_lock(&gfile_lock);
    if (rb_find(name, &global_file, file_find)) {
        spin_unlock(&gfile_lock);
        return -EALREADY;
    }

    length = strnlen(name, PATH_MAX);
    node = kmalloc(sizeof(*node) + length + 1, GFP_KERNEL);
    if (unlikely(!node)) {
        spin_unlock(&gfile_lock);
        return -ENOMEM;
    }

    memcpy(node->name, name, length);
    node->name[length] = '\0';

    path = kbasename(name);
    node->dirlen = path - name - 1;

    rb_add(&node->node, &global_file, file_cmp);
    spin_unlock(&gfile_lock);

    return 0;
}

int
lksu_table_gfile_remove(const char *name)
{
    struct file_table *node;
    struct rb_node *rb;

    if (!*name)
        return -EINVAL;

    spin_lock(&gfile_lock);
    rb = rb_find(name, &global_file, file_find);
    if (!rb) {
        spin_unlock(&gfile_lock);
        return -ENOENT;
    }

    node = node_to_file(rb);
    rb_erase(&node->node, &global_file);
    spin_unlock(&gfile_lock);

    return 0;
}

bool
lksu_table_guid_check(uid_t uid)
{
    struct rb_node *rb;

    spin_lock(&gfile_lock);
    rb = rb_find((void *)(uintptr_t)uid, &global_uid, uid_find);
    spin_unlock(&gfile_lock);

    return !!rb;
}

int
lksu_table_guid_add(uid_t uid)
{
    struct uid_table *node;

    spin_lock(&guid_lock);
    if (rb_find((void *)(uintptr_t)uid, &global_uid, uid_find)) {
        spin_unlock(&guid_lock);
        return -EALREADY;
    }

    node = kmalloc(sizeof(*node), GFP_KERNEL);
    if (unlikely(!node)) {
        spin_unlock(&guid_lock);
        return -ENOMEM;
    }

    node->uid = uid;
    rb_add(&node->node, &global_uid, uid_cmp);
    spin_unlock(&guid_lock);

    return 0;
}

int
lksu_table_guid_remove(uid_t uid)
{
    struct uid_table *node;
    struct rb_node *rb;

    spin_lock(&guid_lock);
    rb = rb_find((void *)(uintptr_t)uid, &global_uid, uid_find);
    if (!rb) {
        spin_unlock(&guid_lock);
        return -ENOENT;
    }

    node = node_to_uid(rb);
    rb_erase(&node->node, &global_file);
    spin_unlock(&guid_lock);

    return 0;
}
