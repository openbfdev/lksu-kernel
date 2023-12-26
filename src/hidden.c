/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#define MODULE_NAME "lksu-hooks"
#define pr_fmt(fmt) MODULE_NAME ": " fmt

#include "hooks.h"
#include "token.h"
#include "uapi.h"

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/printk.h>
#include <linux/rbtree.h>
#include <linux/errname.h>

static struct rb_root hidden_dirent = RB_ROOT;
static DEFINE_SPINLOCK(dirent_lock);

struct iter_context {
	struct dir_context ctx;
    struct dir_context *octx;
};

struct hidden_file {
    struct rb_node node;
    struct file *file;
    struct file_operations *fops;
};

#define node_to_hidden(ptr) \
    rb_entry(ptr, struct hidden_file, node)

static inline char *
inode_path(struct inode *inode, char *buffer, int len, bool *noalias)
{
	struct dentry *dentry;
    char *name;

    *noalias = false;
	dentry = d_find_alias(inode);
	if (!dentry) {
        *noalias = true;
    	return NULL;
    }

	name = dentry_path_raw(dentry, buffer, len);
	dput(dentry);

    return name;
}

static bool
hidden_cmp(struct rb_node *na, const struct rb_node *nb)
{
    const struct hidden_file *ta, *tb;

    ta = node_to_hidden(na);
    tb = node_to_hidden(nb);

    return ta->file < tb->file;
}

static int
hidden_find(const void *key, const struct rb_node *node)
{
    const struct hidden_file *hidden;

    hidden = node_to_hidden(node);
    if (hidden->file == key)
        return 0;

    return (void *)hidden->file < key ? -1 : 1;
}

static bool
filldir(struct dir_context *ctx, const char *name, int namlen,
		loff_t offset, u64 ino, unsigned int d_type)
{
    struct iter_context *ictx;
    struct dir_context *octx;

    if (!strncmp(name, "su", namlen))
        return true;

    ictx = container_of(ctx, struct iter_context, ctx);
    octx = ictx->octx;
    octx->pos = ictx->ctx.pos;

    return octx->actor(octx, name, namlen, offset, ino, d_type);
}

static int
iter_file(struct file *file, struct dir_context *dctx)
{
    struct hidden_file *hidden;
    struct iter_context ictx;
    struct rb_node *rb;
    int retval;

    ictx.ctx.actor = filldir;
    ictx.ctx.pos = dctx->pos;
    ictx.octx = dctx;

    rb = rb_find(file, &hidden_dirent, hidden_find);
    hidden = node_to_hidden(rb);

    retval = hidden->fops->iterate_shared(file, &ictx.ctx);
    dctx->pos = ictx.ctx.pos;

    return retval;
}

static int
release_dirent(struct inode *inode, struct file *file)
{
    struct hidden_file *hidden;
    struct rb_node *rb;

    spin_lock(&dirent_lock);
    rb = rb_find(file, &hidden_dirent, hidden_find);
    BUG_ON(!rb);

    rb_erase(rb, &hidden_dirent);
    spin_unlock(&dirent_lock);

    hidden = node_to_hidden(rb);
    kfree(file->f_op);

    file->f_op = hidden->fops;
    kfree(hidden);

    if (file->f_op->release)
        return file->f_op->release(inode, file);

    return 0;
}

int
lksu_hidden_dirent(struct file *file)
{
    struct file_operations *fops;
    struct hidden_file *hidden;
    char *buffer, *name;
    int retval = 0;

    buffer = __getname();
    if (unlikely(!buffer))
        return -ENOMEM;

    fops = NULL;
    hidden = NULL;

    name = file_path(file, buffer, PATH_MAX);
    if (IS_ERR_OR_NULL(name)) {
        retval = name ? PTR_ERR(name) : -EFAULT;
        goto failed;
    }

    if (strcmp("/usr/bin", name))
        goto failed;

    fops = kmalloc(sizeof(*fops), GFP_KERNEL);
    if (unlikely(!fops)) {
        retval = -ENOMEM;
        goto failed;
    }

    hidden = kmalloc(sizeof(*hidden), GFP_KERNEL);
    if (unlikely(!hidden)) {
        retval = -ENOMEM;
        goto failed;
    }

    *fops = *file->f_op;
    fops->iterate_shared = iter_file;
    fops->release = release_dirent;

    hidden->file = file;
    hidden->fops = (void *)file->f_op;
    file->f_op = fops;

    spin_lock(&dirent_lock);
    rb_add(&hidden->node, &hidden_dirent, hidden_cmp);
    spin_unlock(&dirent_lock);

    __putname(buffer);
    return 0;

failed:
    kfree(hidden);
    kfree(fops);
    __putname(buffer);
    return retval;
}

int
lksu_hidden_file(struct file *file, bool *hidden)
{
    char *buffer, *name;
    int retval = 0;

    buffer = __getname();
    if (unlikely(!buffer))
        return -ENOMEM;

    name = file_path(file, buffer, PATH_MAX);
    if (IS_ERR_OR_NULL(name)) {
        retval = name ? PTR_ERR(name) : -EFAULT;
        goto finish;
    }

    *hidden = false;
    if (!strcmp("/usr/bin/su", name))
        *hidden = true;

finish:
    __putname(buffer);
    return retval;
}

int
lksu_hidden_path(const struct path *path, bool *hidden)
{
    char *buffer, *name;
    int retval = 0;

    buffer = __getname();
    if (unlikely(!buffer))
        return -ENOMEM;

    name = d_path(path, buffer, PATH_MAX);
    if (IS_ERR_OR_NULL(name)) {
        retval = name ? PTR_ERR(name) : -EFAULT;
        goto finish;
    }

    *hidden = false;
    if (!strcmp("/usr/bin/su", name))
        *hidden = true;

finish:
    __putname(buffer);
    return retval;
}

int
lksu_hidden_inode(struct inode *inode, bool *hidden)
{
    char *buffer, *name;
    bool noalias;
    int retval = 0;

    buffer = __getname();
    if (unlikely(!buffer))
        return -ENOMEM;

    *hidden = false;
    name = inode_path(inode, buffer, PATH_MAX, &noalias);
    if (noalias) {
        retval = 0;
        goto finish;
    }

    if (IS_ERR_OR_NULL(name)) {
        retval = name ? PTR_ERR(name) : -EFAULT;
        goto finish;
    }

    if (!strcmp("/usr/bin/su", name))
        *hidden = true;

finish:
    __putname(buffer);
    return retval;
}
