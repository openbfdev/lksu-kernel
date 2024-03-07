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
#include <linux/bug.h>
#include <linux/printk.h>

struct rb_root lksu_global_file = RB_ROOT;
DEFINE_RWLOCK(lksu_gfile_lock);

static struct kmem_cache *guid_cache;
struct rb_root lksu_global_uid = RB_ROOT;
DEFINE_RWLOCK(lksu_guid_lock);

static const char *
const_hidden[] = {
    "/proc/lksu"
};

static inline int
cmp_dirent(const char *file, const char *dirent)
{
    const char *name;

    name = strrchr(file, '/');
    if (WARN_ON(!name))
        return 0;

    return strncmp(file, dirent, name - file);
}

static bool
file_cmp(struct rb_node *na, const struct rb_node *nb)
{
    const struct lksu_file_table *ta, *tb;

    ta = lksu_node_to_file(na);
    tb = lksu_node_to_file(nb);

    return strcmp(ta->name, tb->name) < 0;
}

static int
file_find(const void *key, const struct rb_node *node)
{
    const struct lksu_file_table *table;

    table = lksu_node_to_file(node);

    return strcmp(table->name, key);
}

static int
dirent_find(const void *key, const struct rb_node *node)
{
    const struct lksu_file_table *table;

    table = lksu_node_to_file(node);

    return cmp_dirent(table->name, key);
}

static bool
uid_cmp(struct rb_node *na, const struct rb_node *nb)
{
    const struct lksu_uid_table *ta, *tb;

    ta = lksu_node_to_uid(na);
    tb = lksu_node_to_uid(nb);

    return uid_lt(ta->kuid, tb->kuid);
}

static int
uid_find(const void *key, const struct rb_node *node)
{
    const struct lksu_uid_table *table;
    const kuid_t *kuidp;

    table = lksu_node_to_uid(node);
    kuidp = key;

    if (uid_eq(table->kuid, *kuidp))
        return 0;

    return uid_lt(table->kuid, *kuidp) ? -1 : 1;
}

static bool
const_gfile_check(const char *name)
{
    unsigned int index;

    for (index = 0; index < ARRAY_SIZE(const_hidden); ++index) {
        if (!strcmp(const_hidden[index], name))
            return true;
    }

    return false;
}

static bool
const_gdirent_check(const char *name)
{
    unsigned int index;

    for (index = 0; index < ARRAY_SIZE(const_hidden); ++index) {
        if (!cmp_dirent(const_hidden[index], name))
            return true;
    }

    return false;
}

bool
lksu_table_gfile_check(const char *name)
{
    struct rb_node *rb;

    if (const_gfile_check(name))
        return true;

    read_lock(&lksu_gfile_lock);
    rb = lksu_rb_find(name, &lksu_global_file, file_find);
    read_unlock(&lksu_gfile_lock);

    return !!rb;
}

bool
lksu_table_gdirent_check(const char *name)
{
    struct rb_node *rb;

    if (const_gdirent_check(name))
        return true;

    read_lock(&lksu_gfile_lock);
    rb = lksu_rb_find(name, &lksu_global_file, dirent_find);
    read_unlock(&lksu_gfile_lock);

    return !!rb;
}

int
lksu_table_gfile_add(const char *name)
{
    struct lksu_file_table *node;
    const char *path;
    size_t length;

    if (!*name)
        return -EINVAL;

    write_lock(&lksu_gfile_lock);
    if (lksu_rb_find(name, &lksu_global_file, file_find)) {
        write_unlock(&lksu_gfile_lock);
        return -EALREADY;
    }

    length = strnlen(name, PATH_MAX);
    node = kmalloc(sizeof(*node) + length + 1, GFP_KERNEL);
    if (unlikely(!node)) {
        write_unlock(&lksu_gfile_lock);
        return -ENOMEM;
    }

    memcpy(node->name, name, length);
    node->name[length] = '\0';

    path = kbasename(name);
    node->dirlen = path - name - 1;

    lksu_rb_add(&node->node, &lksu_global_file, file_cmp);
    write_unlock(&lksu_gfile_lock);

    return 0;
}

int
lksu_table_gfile_remove(const char *name)
{
    struct lksu_file_table *node;
    struct rb_node *rb;

    if (!*name)
        return -EINVAL;

    write_lock(&lksu_gfile_lock);
    rb = lksu_rb_find(name, &lksu_global_file, file_find);
    if (!rb) {
        write_unlock(&lksu_gfile_lock);
        return -ENOENT;
    }

    node = lksu_node_to_file(rb);
    rb_erase(&node->node, &lksu_global_file);
    write_unlock(&lksu_gfile_lock);

    kfree(node);

    return 0;
}

bool
lksu_table_guid_check(kuid_t kuid)
{
    struct rb_node *rb;

    read_lock(&lksu_gfile_lock);
    rb = lksu_rb_find(&kuid, &lksu_global_uid, uid_find);
    read_unlock(&lksu_gfile_lock);

    return !!rb;
}

int
lksu_table_guid_add(kuid_t kuid)
{
    struct lksu_uid_table *node;

    write_lock(&lksu_guid_lock);
    if (lksu_rb_find(&kuid, &lksu_global_uid, uid_find)) {
        write_unlock(&lksu_guid_lock);
        return -EALREADY;
    }

    node = kmem_cache_alloc(guid_cache, GFP_KERNEL);
    if (unlikely(!node)) {
        write_unlock(&lksu_guid_lock);
        return -ENOMEM;
    }

    node->kuid = kuid;
    lksu_rb_add(&node->node, &lksu_global_uid, uid_cmp);
    write_unlock(&lksu_guid_lock);

    return 0;
}

int
lksu_table_guid_remove(kuid_t kuid)
{
    struct lksu_uid_table *node;
    struct rb_node *rb;

    write_lock(&lksu_guid_lock);
    rb = lksu_rb_find(&kuid, &lksu_global_uid, uid_find);
    if (!rb) {
        write_unlock(&lksu_guid_lock);
        return -ENOENT;
    }

    node = lksu_node_to_uid(rb);
    rb_erase(&node->node, &lksu_global_file);
    write_unlock(&lksu_guid_lock);

    kmem_cache_free(guid_cache, node);

    return 0;
}

void
lksu_table_flush(void)
{
    struct lksu_file_table *file, *tfile;
    struct lksu_uid_table *uid, *tuid;

    write_lock(&lksu_gfile_lock);
    rbtree_postorder_for_each_entry_safe(file, tfile, &lksu_global_file, node)
        kfree(file);

    lksu_global_file = RB_ROOT;
    write_unlock(&lksu_gfile_lock);

    write_lock(&lksu_guid_lock);
    rbtree_postorder_for_each_entry_safe(uid, tuid, &lksu_global_uid, node)
        kmem_cache_free(guid_cache, uid);

    lksu_global_uid = RB_ROOT;
    write_unlock(&lksu_guid_lock);
}

int __init
lksu_tables_init(void)
{
    guid_cache = KMEM_CACHE(lksu_uid_table, 0);
    if (!guid_cache)
        return -ENOMEM;

    rwlock_init(&lksu_gfile_lock);
    rwlock_init(&lksu_guid_lock);

    return 0;
}

void
lksu_tables_exit(void)
{
    kmem_cache_destroy(guid_cache);
}
