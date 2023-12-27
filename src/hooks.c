/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#define MODULE_NAME "lksu-hooks"
#define pr_fmt(fmt) MODULE_NAME ": " fmt

#include "hooks.h"
#include "token.h"
#include "hidden.h"
#include "lksu.h"

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cred.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/rbtree.h>
#include <linux/errname.h>

static bool enabled __read_mostly;

static int
hook_file_open(struct file *file)
{
    bool hidden;
    int retval;

    if (!enabled)
        return 0;

    retval = lksu_hidden_file(file, &hidden);
    if (unlikely(retval))
        return retval;

    if (hidden)
        return -ENOENT;

    return lksu_hidden_dirent(file);
}

static int
hook_inode_getattr(const struct path *path)
{
    bool hidden;
    int retval;

    if (!enabled)
        return 0;

    retval = lksu_hidden_path(path, &hidden);
    if (unlikely(retval))
        return retval;

    return hidden ? -ENOENT : 0;
}

static int
hook_inode_permission(struct inode *inode)
{
    bool hidden;
    int retval;

    if (!enabled)
        return 0;

    retval = lksu_hidden_inode(inode, &hidden);
    if (unlikely(retval))
        return retval;

    return hidden ? -ENOENT : 0;
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
            pr_notice("module enable\n");
            enabled = true;
            break;

        case LKSU_DISABLE:
            pr_notice("module disable\n");
            enabled = false;
            break;

        case LKSU_GLOBAL_HIDDEN_ADD:

            break;

        case LKSU_GLOBAL_HIDDEN_REMOVE:
            break;

        case LKSU_GLOBAL_UID_ADD:
            break;

        case LKSU_GLOBAL_UID_REMOVE:
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
    pr_warn("unverified operation: %s\n", ename);
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
