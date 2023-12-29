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
#include <linux/version.h>
#include <linux/lsm_hooks.h>
#include <linux/errname.h>

#define LSM_RET_DEFAULT(NAME) (NAME##_default)
#define DECLARE_LSM_RET_DEFAULT_void(DEFAULT, NAME)
#define DECLARE_LSM_RET_DEFAULT_int(DEFAULT, NAME) \
    static const int __maybe_unused LSM_RET_DEFAULT(NAME) = (DEFAULT);
#define LSM_HOOK(RET, DEFAULT, NAME, ...) \
    DECLARE_LSM_RET_DEFAULT_##RET(DEFAULT, NAME)

#include <linux/lsm_hook_defs.h>
#undef LSM_HOOK

static bool enabled __read_mostly;

static inline int
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

static inline int
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

static inline int
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

static inline bool
hook_control(int *retptr, struct lksu_message __user *message)
{
    struct lksu_message msg;
    unsigned long length;
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 8)
    const char *ename;
    ename = errname(retval) ?: "EUNKNOW";
    pr_warn("unverified operation: %s\n", ename);
#else
    pr_warn("unverified operation: %d\n", retval);
#endif

    return false;
}

#if defined(CONFIG_LKSU_HOOK_LSM)
# include "hook-lsm.c"
#elif defined(CONFIG_LKSU_HOOK_LIVEPATCH)
# include "hook-livepatch.c"
#elif defined(CONFIG_LKSU_HOOK_KPROBE)
# include "hook-kprobe.c"
#else
# error "Undefined hook function"
#endif

int __init
lksu_hooks_init(void)
{
#if defined(CONFIG_LKSU_HOOK_LSM)
    return hooks_lsm_init();
#elif defined(CONFIG_LKSU_HOOK_LIVEPATCH)
    return hooks_livepatch_init();
#else
    return hooks_kprobe_init();
#endif
}

void __exit
lksu_hooks_exit(void)
{
#if defined(CONFIG_LKSU_HOOK_LSM)
    hooks_lsm_exit();
#elif defined(CONFIG_LKSU_HOOK_LIVEPATCH)
    hooks_livepatch_exit();
#else
    hooks_kprobe_exit();
#endif
}
