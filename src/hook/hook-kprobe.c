/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#include <linux/kprobes.h>

static int
kprobe_file_open(struct kretprobe_instance *ri, struct pt_regs *regs)
{
#if 1
    struct file *file;
    int retval;

    file = (struct file *)regs_get_kernel_argument(regs, 0);

    retval = hook_file_open(file);
    if (retval)
        regs_set_return_value(regs, retval);

#endif /* For debug */

    return 0;
}

static int
kprobe_inode_getattr(struct kretprobe_instance *ri, struct pt_regs *regs)
{
#if 1
    struct path *path;
    int retval;

    path = (struct path *)regs_get_kernel_argument(regs, 0);
	if (unlikely(IS_PRIVATE(d_backing_inode(path->dentry))))
		return 0;

    retval = hook_inode_getattr(path);
    if (retval)
        regs_set_return_value(regs, retval);

#endif /* For debug */

    return 0;
}

static int
kprobe_inode_permission(struct kretprobe_instance *ri, struct pt_regs *regs)
{
#if 1
    struct inode *inode;
    int retval;

    inode = (struct inode *)regs_get_kernel_argument(regs, 0);
	if (unlikely(IS_PRIVATE(inode)))
		return 0;

    retval = hook_inode_permission(inode);
    if (retval)
        regs_set_return_value(regs, retval);

#endif /* For debug */

    return 0;
}

static int
kprobe_task_prctl(struct kretprobe_instance *ri, struct pt_regs *regs)
{
#if 1
    unsigned long arg2;
    int option, retval;

    option = (int)regs_get_kernel_argument(regs, 0);
    arg2 = (unsigned long)regs_get_kernel_argument(regs, 1);

    if (option != LKSU_SYSCALL_CTLKEY)
        return 0;

    if (!hook_control(&retval, (void __user *)arg2))
        return 0;

    regs_set_return_value(regs, retval);

#endif /* For debug */

    return 0;
}

static struct kretprobe *
kprobe_hooks[] = {
    &(struct kretprobe) {
        .kp.symbol_name = "security_file_open",
        .handler = kprobe_file_open,
    },
    &(struct kretprobe) {
        .kp.symbol_name = "security_inode_getattr",
        .handler = kprobe_inode_getattr,
    },
    &(struct kretprobe) {
        .kp.symbol_name = "security_inode_permission",
        .handler = kprobe_inode_permission,
    },
    &(struct kretprobe) {
        .kp.symbol_name = "security_task_prctl",
        .handler = kprobe_task_prctl,
    },
};

static int
hooks_kprobe_init(void)
{
    pr_notice("used kprobe function\n");
    return register_kretprobes(kprobe_hooks, ARRAY_SIZE(kprobe_hooks));
}

static void
hooks_kprobe_exit(void)
{
    unregister_kretprobes(kprobe_hooks, ARRAY_SIZE(kprobe_hooks));
}
