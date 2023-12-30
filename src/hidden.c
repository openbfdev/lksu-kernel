/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#define MODULE_NAME "lksu-hooks"
#define pr_fmt(fmt) MODULE_NAME ": " fmt

#include "lksu.h"
#include "hidden.h"
#include "tables.h"

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/printk.h>
#include <linux/rbtree.h>
#include <linux/errname.h>

static struct kmem_cache *filldir_cache __read_mostly;
static struct rb_root hidden_dirent = RB_ROOT;
static DEFINE_SPINLOCK(dirent_lock);

struct iter_context {
	struct dir_context ctx;
    struct dir_context *octx;
    const char *path;
    char *name;
};

struct hidden_dirent {
    struct rb_node node;
    struct file *file;
    struct file_operations *fops;
};

#define node_to_hidden(ptr) \
    rb_entry(ptr, struct hidden_dirent, node)

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
    const struct hidden_dirent *ta, *tb;

    ta = node_to_hidden(na);
    tb = node_to_hidden(nb);

    return ta->file < tb->file;
}

static int
hidden_find(const void *key, const struct rb_node *node)
{
    const struct hidden_dirent *hidden;

    hidden = node_to_hidden(node);
    if (hidden->file == key)
        return 0;

    return (void *)hidden->file < key ? -1 : 1;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
# define FILLDIR_RET bool
#else
# define FILLDIR_RET int
#endif

static FILLDIR_RET
filldir(struct dir_context *ctx, const char *name, int namlen,
        loff_t offset, u64 ino, unsigned int d_type)
{
    struct iter_context *ictx;
    struct dir_context *octx;

    ictx = container_of(ctx, struct iter_context, ctx);
    strcpy(ictx->name, name);

    if (lksu_table_gfile_check(ictx->path))
        return true;

    octx = ictx->octx;
    octx->pos = ictx->ctx.pos;

    return octx->actor(octx, name, namlen, offset, ino, d_type);
}

static int
iter_file(struct file *file, struct dir_context *dctx)
{
    struct hidden_dirent *hidden;
    struct iter_context ictx;
    struct rb_node *rb;
    char *buffer, *name;
    int retval;

    spin_lock(&dirent_lock);
    rb = rb_find(file, &hidden_dirent, hidden_find);
    BUG_ON(!rb);
    spin_unlock(&dirent_lock);

    buffer = kmem_cache_alloc(filldir_cache, GFP_KERNEL);
    if (unlikely(!buffer))
        return -ENOMEM;

    name = file_path(file, buffer, PATH_MAX);
    if ((retval = PTR_ERR_OR_ZERO(name)))
        goto finish;

    ictx.ctx.actor = filldir;
    ictx.ctx.pos = dctx->pos;
    ictx.octx = dctx;

    ictx.path = name;
    ictx.name = name + strlen(name);
    *ictx.name++ = '/';

    hidden = node_to_hidden(rb);
    retval = hidden->fops->iterate_shared(file, &ictx.ctx);
    dctx->pos = ictx.ctx.pos;

finish:
    kmem_cache_free(filldir_cache, buffer);
    return retval;
}

static int
release_dirent(struct inode *inode, struct file *file)
{
    struct hidden_dirent *hidden;
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
    struct hidden_dirent *hidden;
    char *buffer, *name;
    int retval;

    buffer = __getname();
    if (unlikely(!buffer))
        return -ENOMEM;

    fops = NULL;
    hidden = NULL;

    name = file_path(file, buffer, PATH_MAX);
    if ((retval = PTR_ERR_OR_ZERO(name)))
        goto exit;

    if (!lksu_table_gdirent_check(name))
        goto exit;

    fops = kmalloc(sizeof(*fops), GFP_KERNEL);
    if (unlikely(!fops)) {
        retval = -ENOMEM;
        goto exit;
    }

    hidden = kmalloc(sizeof(*hidden), GFP_KERNEL);
    if (unlikely(!hidden)) {
        retval = -ENOMEM;
        goto exit;
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

exit:
    kfree(hidden);
    kfree(fops);
    __putname(buffer);
    return retval;
}

int
lksu_hidden_file(struct file *file, bool *hidden)
{
    char *buffer, *name;
    int retval;

    *hidden = false;

    buffer = __getname();
    if (unlikely(!buffer))
        return -ENOMEM;

    name = file_path(file, buffer, PATH_MAX);
    if ((retval = PTR_ERR_OR_ZERO(name)))
        goto finish;

    if (lksu_table_gfile_check(name))
        *hidden = true;

finish:
    __putname(buffer);
    return retval;
}

int
lksu_hidden_path(const struct path *path, bool *hidden)
{
    char *buffer, *name;
    int retval;

    *hidden = false;

    buffer = __getname();
    if (unlikely(!buffer))
        return -ENOMEM;

    name = d_path(path, buffer, PATH_MAX);
    if ((retval = PTR_ERR_OR_ZERO(name)))
        goto finish;

    if (lksu_table_gfile_check(name))
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

    *hidden = false;

    buffer = __getname();
    if (unlikely(!buffer))
        return -ENOMEM;

    name = inode_path(inode, buffer, PATH_MAX, &noalias);
    if (noalias) {
        retval = 0;
        goto finish;
    }

    if (IS_ERR_OR_NULL(name)) {
        retval = name ? PTR_ERR_OR_ZERO(name) : -EFAULT;
        goto finish;
    }

    if (lksu_table_gfile_check(name))
        *hidden = true;

finish:
    __putname(buffer);
    return retval;
}

int
lksu_hidden_init(void)
{
	filldir_cache = kmem_cache_create(
        "names_cache", PATH_MAX + NAME_MAX + 1, 0,
        SLAB_HWCACHE_ALIGN | SLAB_PANIC, NULL
    );

    if (!filldir_cache)
        return -ENOMEM;

    return 0;
}

void
lksu_hidden_exit(void)
{

}
