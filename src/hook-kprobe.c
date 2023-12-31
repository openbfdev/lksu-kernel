/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#include <linux/kprobes.h>

struct kprobe_args {
    unsigned long data[2];
};

static int
kprobe_get_args(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct kprobe_args *args;

    args = (void *)ri->data;
    args->data[0] = regs_get_kernel_argument(regs, 0);
    args->data[1] = regs_get_kernel_argument(regs, 1);

    return 0;
}

static int
kprobe_file_open(struct kretprobe_instance *ri, struct pt_regs *regs)
{
#if 1
    struct kprobe_args *args;
    struct file *file;
    int retval;

    args = (void *)ri->data;
    file = (struct file *)args->data[0];

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
    struct kprobe_args *args;
    struct path *path;
    int retval;

    args = (void *)ri->data;
    path = (struct path *)args->data[0];

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
    struct kprobe_args *args;
    struct inode *inode;
    int retval;

    args = (void *)ri->data;
    inode = (struct inode *)args->data[0];

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
    struct kprobe_args *args;
    struct lksu_message __user *message;
    int option, retval;

    args = (void *)ri->data;
    option = (int)args->data[0];
    message = (void __user *)args->data[1];

    if (option != LKSU_SYSCALL_CTLKEY)
        return 0;

    if (!hook_control(&retval, message))
        return 0;

    regs_set_return_value(regs, retval);

#endif /* For debug */

    return 0;
}

static struct kretprobe *
kprobe_hooks[] = {
    &(struct kretprobe) {
        .kp.symbol_name = "security_file_open",
        .entry_handler = kprobe_get_args,
        .handler = kprobe_file_open,
        .data_size = sizeof(struct kprobe_args),
    },
    &(struct kretprobe) {
        .kp.symbol_name = "security_inode_getattr",
        .entry_handler = kprobe_get_args,
        .handler = kprobe_inode_getattr,
        .data_size = sizeof(struct kprobe_args),
    },
    &(struct kretprobe) {
        .kp.symbol_name = "security_inode_permission",
        .entry_handler = kprobe_get_args,
        .handler = kprobe_inode_permission,
        .data_size = sizeof(struct kprobe_args),
    },
    &(struct kretprobe) {
        .kp.symbol_name = "security_task_prctl",
        .entry_handler = kprobe_get_args,
        .handler = kprobe_task_prctl,
        .data_size = sizeof(struct kprobe_args),
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
