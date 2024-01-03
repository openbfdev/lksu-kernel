/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#define MODULE_NAME "lksu-hooks"
#define pr_fmt(fmt) MODULE_NAME ": " fmt

#include "lksu.h"
#include "hooks.h"
#include "token.h"
#include "hidden.h"
#include "tables.h"

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
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

static inline bool
hook_whitelist(void)
{
    if (!READ_ONCE(enabled))
        return true;

    if (lksu_table_guid_check(current_uid()))
        return true;

    return false;
}

static int
hook_file_open(struct file *file)
{
    bool hidden;
    int retval;

    if (hook_whitelist())
        return 0;

    if (file->f_flags & O_DIRECTORY)
        return lksu_hidden_dirent(file);

    retval = lksu_hidden_file(file, &hidden);
    if (unlikely(retval))
        return retval;

    return hidden ? -ENOENT : 0;
}

static int
hook_inode_getattr(const struct path *path)
{
    bool hidden;
    int retval;

    if (hook_whitelist())
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

    if (hook_whitelist())
        return 0;

    retval = lksu_hidden_inode(inode, &hidden);
    if (unlikely(retval))
        return retval;

    return hidden ? -ENOENT : 0;
}

static const char *
hook_copy_path(const char __user *name)
{
    char *buffer;
    long retlen;

    buffer = __getname();
    if (unlikely(!buffer))
        return NULL;

    retlen = strncpy_from_user(buffer, name, PATH_MAX);
    if (unlikely(retlen < 0)) {
        __putname(buffer);
        return NULL;
    }

    return buffer;
}

static bool
hook_control(int *retptr, struct lksu_message __user *message)
{
    struct lksu_message msg;
    unsigned long length;
    bool verify;
    int retval;

    verify = false;
    retval = 0;

    if (unlikely(!message)) {
        retval = -EINVAL;
        goto finish;
    }

    length = copy_from_user(&msg, message, sizeof(msg));
    if (unlikely(length)) {
        retval = -EFAULT;
        goto finish;
    }

    verify = lksu_token_verify(msg.token);
    if (unlikely(!verify)) {
        retval = -EACCES;
        goto finish;
    }

    switch (msg.func) {
        case LKSU_ENABLE:
            pr_notice("module enable\n");
            WRITE_ONCE(enabled, true);
            break;

        case LKSU_DISABLE:
            pr_notice("module disable\n");
            WRITE_ONCE(enabled, false);
            break;

        case LKSU_GLOBAL_HIDDEN_ADD: {
            const char *hidden;

            hidden = hook_copy_path((void __user *)msg.args.g_hidden);
            if (unlikely(!hidden)) {
                retval = -ENOMEM;
                break;
            }

            pr_notice("global file add: %s\n", hidden);
            retval = lksu_table_gfile_add(hidden);
            __putname(hidden);
            break;
        }

        case LKSU_GLOBAL_HIDDEN_REMOVE: {
            const char *hidden;

            hidden = hook_copy_path((void __user *)msg.args.g_hidden);
            if (unlikely(!hidden)) {
                retval = -ENOMEM;
                break;
            }

            pr_notice("global file remove: %s\n", hidden);
            retval = lksu_table_gfile_remove(hidden);
            __putname(hidden);
            break;
        }

        case LKSU_GLOBAL_UID_ADD: {
            kuid_t kuid;

            kuid = make_kuid(current_user_ns(), msg.args.g_uid);
            if (!uid_valid(kuid)) {
                retval = -EINVAL;
                break;
            }

            pr_notice("global uid add: %u\n", __kuid_val(kuid));
            retval = lksu_table_guid_add(kuid);
            break;
        }

        case LKSU_GLOBAL_UID_REMOVE: {
            kuid_t kuid;

            kuid = make_kuid(current_user_ns(), msg.args.g_uid);
            if (!uid_valid(kuid)) {
                retval = -EINVAL;
                break;
            }

            pr_notice("global uid remove: %u\n", __kuid_val(kuid));
            retval = lksu_table_guid_add(kuid);
            break;
        }

        case LKSU_TOKEN_ADD:
            pr_notice("token add: %.*s\n", LKSU_TOKEN_LEN, msg.args.token);
            retval = lksu_token_add(msg.args.token);
            break;

        case LKSU_TOKEN_REMOVE:
            pr_notice("token remove: %.*s\n", LKSU_TOKEN_LEN, msg.args.token);
            retval = lksu_token_remove(msg.args.token);
            break;

        default:
            retval = -EINVAL;
            break;
    }

finish:
    *retptr = retval;

    if (retval) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 7, 0)
        const char *ename;
        ename = errname(retval) ?: "EUNKNOW";
        pr_warn("illegal operation: %s (%s)\n", ename,
                verify ? "verified" : "non-validated");
#else
        pr_warn("illegal operation: %d (%s)\n", retval,
                verify ? "verified" : "non-validated");
#endif
    }

    return verify;
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

int
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

void
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
