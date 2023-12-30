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

bool hook_control_enable(struct lksu_message *msg, int *retptr);
bool hook_control_disable(struct lksu_message *msg, int *retptr);
bool hook_control_global_hidden_add(struct lksu_message *msg, int *retptr);
bool hook_control_global_hidden_remove(struct lksu_message *msg, int *retptr);
bool hook_control_global_uid_add(struct lksu_message *msg, int *retptr);
bool hook_control_global_uid_remove(struct lksu_message *msg, int *retptr);
bool hook_control_token_add(struct lksu_message *msg, int *retptr);
bool hook_control_token_remove(struct lksu_message *msg, int *retptr);


static bool enabled __read_mostly;


static struct lksu_control_func funcs[] = {
    {
        LKSU_ENABLE,
        hook_control_enable,
    },
    {
        LKSU_DISABLE,
        hook_control_disable,
    },
    {
        LKSU_GLOBAL_HIDDEN_ADD,
        hook_control_global_hidden_add,
    },
    {
        LKSU_GLOBAL_HIDDEN_REMOVE,
        hook_control_global_hidden_remove,
    },
    {
        LKSU_GLOBAL_UID_ADD,
        hook_control_global_uid_add,
    },
    {
        LKSU_GLOBAL_UID_REMOVE,
        hook_control_global_uid_remove,
    },
    {
        LKSU_TOKEN_ADD,
        hook_control_token_add,
    },
    {
        LKSU_TOKEN_REMOVE,
        hook_control_token_remove,
    },
};


static inline int
hook_file_open(struct file *file)
{
    bool hidden;
    int retval;

    if (!enabled)
        return 0;

    if (lksu_table_guid_check(current_uid().val))
        return 0;

    if (file->f_flags & O_DIRECTORY)
        return lksu_hidden_dirent(file);

    retval = lksu_hidden_file(file, &hidden);
    if (unlikely(retval))
        return retval;

    if (hidden)
        return -ENOENT;

    return 0;
}

static inline int
hook_inode_getattr(const struct path *path)
{
    bool hidden;
    int retval;

    if (!enabled)
        return 0;

    if (lksu_table_guid_check(current_uid().val))
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

    if (lksu_table_guid_check(current_uid().val))
        return 0;

    retval = lksu_hidden_inode(inode, &hidden);
    if (unlikely(retval))
        return retval;

    return hidden ? -ENOENT : 0;
}

static inline char* hook_crontol_new_path_buffer(struct lksu_message *msg, int *retptr) {
    char *buffer;
    long retlen;
    //this is a api???. fuck
    buffer = __getname();

    if (!buffer) {
        *retptr = -ENOMEM;
        return NULL;
    }

    retlen = strncpy_from_user(buffer, msg.args.g_hidden, PATH_MAX);
    if (unlikely(retlen < 0)) {
        __putname(buffer);
        *retptr = retlen;
        return NULL;
    }

    return buffer;
}

bool hook_control_enable(struct lksu_message *msg, int *retptr) {
    pr_notice("module enable\n");
    enabled = true;
    return true;
}

bool hook_control_enable(struct lksu_message *msg, int *retptr) {
    pr_notice("module disable\n");
    enabled = false;
    return true;
}

bool hook_control_global_hidden_add(struct lksu_message *msg, int *retptr) {
    char *buf;
    int retval = 0;

    buf = hook_crontol_new_path_buffer(msg, retptr);
    if (buf == NULL) {
        return false;
    }

    pr_notice("global file add: %s\n", buffer);
    retval = lksu_table_gfile_add(buffer);
    if (retval != 0) {
        *retptr = retval;
        return false;
    }

    __putname(buffer);
    return true;
}

bool hook_control_global_hidden_remove(struct lksu_message *msg, int *retptr) {
    char *buf;
    int retval = 0;

    buf = hook_crontol_new_path_buffer(msg, retptr);
    if (buf == NULL) {
        return false;
    }

    pr_notice("global file remove: %s\n", buffer);
    retval = lksu_table_gfile_remove(buffer);
    if (retval != 0) {
        *retptr = retval;
        return false;
    }

    __putname(buffer);
    return true;
}

bool hook_control_global_uid_add(struct lksu_message *msg, int *retptr) {
    pr_notice("global uid add: %u\n", msg.args.g_uid);
    int retval = 0;

    retval = lksu_table_guid_add(msg.args.g_uid);
    if (retval != 0) {
        *retptr = retval;
        return false;
    }
    return true;
}

bool hook_control_global_uid_remove(struct lksu_message *msg, int *retptr) {
    pr_notice("global uid remove: %u\n", msg.args.g_uid);
    int retval = 0;

    retval = lksu_table_guid_remove(msg.args.g_uid);
    if (retval != 0) {
        *retptr = retval;
        return false;
    }

    return true;
}

bool hook_control_token_add(struct lksu_message *msg, int *retptr) {
    pr_notice("token add: %*.s\n", LKSU_TOKEN_LEN, msg.args.token);
    int retval = 0;

    retval = lksu_token_add(msg.args.token);
    if (retval != 0) {
        *retptr = retval;
        return false;
    }

    return true;
}

bool hook_control_token_remove(struct lksu_message *msg, int *retptr) {
    pr_notice("token remove: %*.s\n", LKSU_TOKEN_LEN, msg.args.token);
    int retval = 0;

    retval = lksu_token_remove(msg.args.token);
    if (retval != 0) {
        *retptr = retval;
        return false;
    }

    return true;
}

static inline bool
hook_control(int *retptr, struct lksu_message __user *message)
{
    struct lksu_message msg;
    unsigned long length;
    int retval = 0;
    bool verify;
    int i;
    lksu_control_func *func;

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

    if (msg.func >= LKSU_FUNC_MAX_NR || msg.func < LKSU_ENABLE) {
        goto failed;
    }

    for (i = 0; ARRAY_SIZE(funcs); i++) {
        func = &funcs[i];
        return func->handler(msg, retptr);
    }

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
