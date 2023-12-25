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
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/rbtree.h>
#include <linux/errname.h>

static bool enabled;
static struct rb_root hidden_files = RB_ROOT;

struct hidden_file {
    struct rb_node node;
    struct file *file;
    struct file_operations *fops;
};

#define node_to_hidden(ptr) \
    rb_entry(ptr, struct hidden_file, node)

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

struct iter_context {
	struct dir_context ctx;
    struct dir_context *octx;
};

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

    rb = rb_find(file, &hidden_files, hidden_find);
    hidden = node_to_hidden(rb);

    retval = hidden->fops->iterate_shared(file, &ictx.ctx);
    dctx->pos = ictx.ctx.pos;

    return retval;
}

static int
hook_file_open(struct file *file)
{
    struct file_operations *fops;
    struct hidden_file *hidden;
    char *buffer, *path;

    buffer = __getname();
    path = file_path(file, buffer, PATH_MAX);

    if (!strcmp("/usr/bin/su", path)) {
        kfree(buffer);
        return -ENOENT;
    }

    if (strcmp("/usr/bin", path)) {
        kfree(buffer);
        return 0;
    }

    __putname(buffer);
    fops = kmalloc(sizeof(*fops), GFP_KERNEL);
    hidden = kmalloc(sizeof(*hidden), GFP_KERNEL);

    *fops = *file->f_op;
    fops->iterate_shared = iter_file;

    hidden->file = file;
    hidden->fops = (void *)file->f_op;

    file->f_op = fops;
    rb_add(&hidden->node, &hidden_files, hidden_cmp);

    return 0;
}

static int
hook_inode_getattr(const struct path *dpath)
{
    char *buffer, *path;

    buffer = __getname();
    path = d_path(dpath, buffer, PATH_MAX);

    if (!strcmp("/usr/bin/su", path)) {
        __putname(buffer);
        return -ENOENT;
    }

    __putname(buffer);

    return 0;
}

static void
hook_file_free(struct file *file)
{
    struct hidden_file *hidden;
    struct rb_node *rb;

    rb = rb_find(file, &hidden_files, hidden_find);
    if (!rb)
        return;

    hidden = node_to_hidden(rb);
    kfree(file->f_op);
    file->f_op = hidden->fops;

    rb_erase(rb, &hidden_files);
    kfree(hidden);
}

static bool
hook_control(int *retptr, struct lksu_message __user *message)
{
    struct lksu_message msg;
    unsigned long length;
    const char *ename;
    bool verify;
    int retval;

    if (!message) {
        retval = -EINVAL;
        goto failed;
    }

    length = copy_from_user(&msg, message, sizeof(msg));
    if (length) {
        retval = -EFAULT;
        goto failed;
    }

    verify = lksu_token_verify(msg.token);
    if (!verify) {
        retval = -EACCES;
        goto failed;
    }

    switch (msg.func) {
        case LKSU_ENABLE:
            enabled = true;
            break;

        case LKSU_DISABLE:
            enabled = false;
            break;

        case LKSU_TOKEN_ADD:
            pr_notice("token add: %*.s\n", LKSU_TOKEN_LEN, msg.args.token);
            retval = lksu_token_add(msg.args.token);
            break;

        case LKSU_TOKEN_REMOVE:
            pr_notice("token remove: %*.s\n", LKSU_TOKEN_LEN, msg.args.token);
            retval = lksu_token_remove(msg.args.token);
            break;

        default:
            goto failed;
    }

    *retptr = retval;
    return true;

failed:
    ename = errname(retval) ?: "EUNKNOW";
    pr_warn("Illegal operation: %s\n", ename);
    return false;
}

#ifdef MODULE
# include "hook-kprobe.c"
#else
# include "hook-lsm.c"
#endif

int __init
lksu_hooks_init(void)
{
#ifdef MODULE
    return hooks_kprobe_init();
#else
    return hooks_lsm_init();
#endif
}

void __exit
lksu_hooks_exit(void)
{
#ifdef MODULE
    hooks_kprobe_exit();
#else
    hooks_lsm_exit();
#endif
}
