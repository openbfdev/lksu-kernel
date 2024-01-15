/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#include <linux/livepatch.h>
#include <linux/fsnotify.h>

static int
livepatch_file_open(struct file *file)
{
    int retval;

#if 1
    retval = hook_file_open(file);
#else /* For debug */
    (void)hook_file_open;
    retval = 0;
#endif

    if (retval)
        return retval;

    return fsnotify_perm(file, MAY_OPEN);
}

static int
livepatch_inode_getattr(const struct path *path)
{
    if (unlikely(IS_PRIVATE(d_backing_inode(path->dentry))))
        return 0;

#if 1
    return hook_inode_getattr(path);
#else /* For debug */
    (void)hook_inode_getattr;
    return 0;
#endif
}

static int
livepatch_inode_permission(struct inode *inode, int mask)
{
    if (unlikely(IS_PRIVATE(inode)))
        return 0;

#if 1
    return hook_inode_permission(inode);
#else /* For debug */
    (void)hook_inode_permission;
    return 0;
#endif
}

static int
livepatch_task_prctl(int option, unsigned long arg2, unsigned long arg3,
                     unsigned long arg4, unsigned long arg5)
{
    int retval;

    if (option != LKSU_SYSCALL_CTLKEY)
        return LSM_RET_DEFAULT(task_prctl);

    if (!hook_control(&retval, (void __user *)arg2))
        return LSM_RET_DEFAULT(task_prctl);

    return retval;
}

/* TODO: Avoid replacing LSM */
static struct klp_func
livepatch_hooks[] = {
    {
        .old_name = "security_file_open",
        .new_func = livepatch_file_open,
    },
    {
        .old_name = "security_inode_getattr",
        .new_func = livepatch_inode_getattr,
    },
    {
        .old_name = "security_inode_permission",
        .new_func = livepatch_inode_permission,
    },
    {
        .old_name = "security_task_prctl",
        .new_func = livepatch_task_prctl,
    },
    { }, /* NULL */
};

static struct klp_object
livepatch_objs[] = {
    {
        /* name being NULL means vmlinux */
        .funcs = livepatch_hooks,
    },
    { }, /* NULL */
};

static struct klp_patch
patch = {
    .mod = THIS_MODULE,
    .objs = livepatch_objs,
    .replace = true,
};

static __init int
hooks_livepatch_init(void)
{
    pr_notice("used livepatch function\n");
    return klp_enable_patch(&patch);
}

static void
hooks_livepatch_exit(void)
{
    /* Never reached */
}

MODULE_INFO(livepatch, "Y");
