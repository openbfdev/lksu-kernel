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
#include <linux/errname.h>

static int
hook_open(struct file *file)
{
    return 0;
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
