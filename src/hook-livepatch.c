/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#include <linux/fsnotify.h>
#include <linux/livepatch.h>

#define call_void_hook(FUNC, ...)                                   \
    do {                                                            \
        struct security_hook_list *P;                               \
        hlist_for_each_entry(P, &security_hook_heads.FUNC, list)    \
                             P->hook.FUNC(__VA_ARGS__);             \
    } while (0)

#define call_int_hook(FUNC, IRC, ...) ({                            \
    int RC = IRC;                                                   \
    do {                                                            \
        struct security_hook_list *P;                               \
        hlist_for_each_entry(P, &security_hook_heads.FUNC, list) {  \
            RC = P->hook.FUNC(__VA_ARGS__);                         \
            if (RC != 0)                                            \
                break;                                              \
        }                                                           \
    } while (0);                                                    \
    RC;                                                             \
})

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

    retval = call_int_hook(file_open, 0, file);
    if (retval)
        return retval;

    return fsnotify_perm(file, MAY_OPEN);
}

static int
livepatch_inode_getattr(const struct path *path)
{
    int retval;

    if (unlikely(IS_PRIVATE(d_backing_inode(path->dentry))))
        return 0;

#if 1
    return hook_inode_getattr(path);
#else /* For debug */
    (void)hook_inode_getattr;
    retval = 0;
#endif

    if (retval)
        return retval;

    return call_int_hook(inode_getattr, 0, path);
}

static int
livepatch_inode_permission(struct inode *inode, int mask)
{
    int retval;

    if (unlikely(IS_PRIVATE(inode)))
        return 0;

#if 1
    retval = hook_inode_permission(inode);
#else /* For debug */
    (void)hook_inode_permission;
    retval = 0;
#endif

    if (retval)
        return retval;

    return call_int_hook(inode_permission, 0, inode, mask);
}

static int
livepatch_task_prctl(int option, unsigned long arg2, unsigned long arg3,
                     unsigned long arg4, unsigned long arg5)
{
    int retval, rc = LSM_RET_DEFAULT(task_prctl);
    struct security_hook_list *hp;

    if (option != LKSU_SYSCALL_CTLKEY)
        return LSM_RET_DEFAULT(task_prctl);

    if (!hook_control(&retval, (void __user *)arg2))
        return LSM_RET_DEFAULT(task_prctl);

    if (retval)
        return retval;

    hlist_for_each_entry(hp, &security_hook_heads.task_prctl, list) {
        retval = hp->hook.task_prctl(option, arg2, arg3, arg4, arg5);
        if (retval != LSM_RET_DEFAULT(task_prctl)) {
            rc = retval;
            if (retval != 0)
                break;
        }
    }

    return rc;
}

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

static int
hooks_livepatch_init(void)
{
    pr_notice("used livepatch function\n");

    security_hook_heads =
    if (!security_hook_heads)
        return -ENOENT;

    return klp_enable_patch(&patch);
}

static void
hooks_livepatch_exit(void)
{
    /* Never reached */
}
