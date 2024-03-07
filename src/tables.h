/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _LKSU_TABLES_H_
#define _LKSU_TABLES_H_

#include <linux/module.h>
#include <linux/types.h>
#include "rbtree.h"

struct lksu_file_table {
    struct rb_node node;
    size_t dirlen;
    char name[];
};

struct lksu_uid_table {
    struct rb_node node;
    kuid_t kuid;
};

#define lksu_node_to_file(ptr) \
    rb_entry(ptr, struct lksu_file_table, node)

#define lksu_node_to_uid(ptr) \
    rb_entry(ptr, struct lksu_uid_table, node)

extern struct rb_root lksu_global_file;
extern rwlock_t lksu_gfile_lock;

extern struct rb_root lksu_global_uid;
extern rwlock_t lksu_guid_lock;

extern bool
lksu_table_gfile_check(const char *name);

extern bool
lksu_table_gdirent_check(const char *name);

extern int
lksu_table_gfile_add(const char *name);

extern int
lksu_table_gfile_remove(const char *name);

extern bool
lksu_table_guid_check(kuid_t kuid);

extern int
lksu_table_guid_add(kuid_t kuid);

extern int
lksu_table_guid_remove(kuid_t kuid);

extern void
lksu_table_flush(void);

extern int
lksu_tables_init(void);

extern void
lksu_tables_exit(void);

#endif /* _LKSU_TABLES_H_ */
