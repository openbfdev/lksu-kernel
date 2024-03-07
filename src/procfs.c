/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2024 John Sanpe <sanpeqf@gmail.com>
 */

#define MODULE_NAME "lksu-procfs"
#define pr_fmt(fmt) MODULE_NAME ": " fmt

#include "lksu.h"
#include "tables.h"
#include "procfs.h"

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/printk.h>

static struct proc_dir_entry *proc_entry;

static int
procfs_show(struct seq_file *seq, void *val)
{
    struct rb_node *rb;

    seq_puts(seq, "global hidden files:\n");
    read_lock(&lksu_gfile_lock);
    for (rb = rb_first(&lksu_global_file); rb; rb = rb_next(rb)) {
        struct lksu_file_table *file;

        file = lksu_node_to_file(rb);
        seq_printf(seq, "\t%s\n", file->name);
    }
    read_unlock(&lksu_gfile_lock);
    seq_puts(seq, "\n");

    seq_puts(seq, "global whitelist uids:\n");
    read_lock(&lksu_guid_lock);
    for (rb = rb_first(&lksu_global_uid); rb; rb = rb_next(rb)) {
        struct lksu_uid_table *uid;
        uid_t value;

        uid = lksu_node_to_uid(rb);
        value = from_kuid(current_user_ns(), uid->kuid);
        seq_printf(seq, "\t%d\n", value);
    }
    read_unlock(&lksu_guid_lock);
    seq_puts(seq, "\n");

    return 0;
}

static int
procfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, procfs_show, NULL);
}

static const struct proc_ops
procfs_ops = {
    .proc_open = procfs_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release_private,
    .proc_release = single_release,
};

int __init
lksu_procfs_init(void)
{
    proc_entry = proc_create("lksu", 0440, NULL, &procfs_ops);
    if (!proc_entry)
        return -ENOMEM;

    return 0;
}

void
lksu_procfs_exit(void)
{
    proc_remove(proc_entry);
}
