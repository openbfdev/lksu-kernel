/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

static int
lsm_file_open(struct file *file)
{
#if 1
    return hook_file_open(file);
#else /* For debug */
    (void)hook_file_open;
    return 0;
#endif
}

static int
lsm_inode_getattr(const struct path *path)
{
#if 1
    return hook_inode_getattr(path);
#else /* For debug */
    (void)hook_inode_getattr;
    return 0;
#endif
}

static int
lsm_inode_permission(struct inode *inode, int mask)
{
#if 1
    return hook_inode_permission(inode);
#else /* For debug */
    (void)hook_inode_permission;
    return 0;
#endif
}

static int
lsm_task_prctl(int option, unsigned long arg2, unsigned long arg3,
               unsigned long arg4, unsigned long arg5)
{
    int retval;

    if (option != LKSU_SYSCALL_CTLKEY)
        return LSM_RET_DEFAULT(task_prctl);

    if (!hook_control(&retval, (void __user *)arg2))
        return LSM_RET_DEFAULT(task_prctl);

    return retval;
}

static struct security_hook_list
lsm_hooks[] = {
    LSM_HOOK_INIT(file_open, lsm_file_open),
    LSM_HOOK_INIT(inode_getattr, lsm_inode_getattr),
    LSM_HOOK_INIT(inode_permission, lsm_inode_permission),
    LSM_HOOK_INIT(task_prctl, lsm_task_prctl),
};

static int
hooks_lsm_init(void)
{
    pr_notice("used lsm function\n");
    security_add_hooks(lsm_hooks, ARRAY_SIZE(lsm_hooks), "lksu");
    return 0;
}

static void
hooks_lsm_exit(void)
{
    /* Never reached */
}
