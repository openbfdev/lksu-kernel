/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#include <linux/kprobes.h>

static int
kprobe_get_args(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct kretprobe *probe;
    unsigned long *args;
    unsigned int count;

    probe = get_kretprobe(ri);
    args = (void *)ri->data;

    for (count = 0; count < probe->data_size / sizeof(unsigned long); ++count)
        args[count] = regs_get_kernel_argument(regs, count);

    WARN_ONCE(
        probe->nmissed,
        "%s: hook event leakage", probe->kp.symbol_name
    );

    return 0;
}

static int
kprobe_file_open(struct kretprobe_instance *ri, struct pt_regs *regs)
{
#if 1
    unsigned long *args;
    struct file *file;
    int retval;

    args = (void *)ri->data;
    file = (struct file *)args[0];

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
    unsigned long *args;
    struct path *path;
    int retval;

    args = (void *)ri->data;
    path = (struct path *)args[0];

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
    unsigned long *args;
    struct inode *inode;
    int retval;

    args = (void *)ri->data;
    inode = (struct inode *)args[0];

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
    unsigned long *args;
    struct lksu_message __user *message;
    int option, retval;

    args = (void *)ri->data;
    option = (int)args[0];
    message = (void __user *)args[1];

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
        .data_size = sizeof(unsigned long [1]),
    },
    &(struct kretprobe) {
        .kp.symbol_name = "security_inode_getattr",
        .entry_handler = kprobe_get_args,
        .handler = kprobe_inode_getattr,
        .data_size = sizeof(unsigned long [1]),
    },
    &(struct kretprobe) {
        .kp.symbol_name = "security_inode_permission",
        .entry_handler = kprobe_get_args,
        .handler = kprobe_inode_permission,
        .data_size = sizeof(unsigned long [1]),
    },
    &(struct kretprobe) {
        .kp.symbol_name = "security_task_prctl",
        .entry_handler = kprobe_get_args,
        .handler = kprobe_task_prctl,
        .data_size = sizeof(unsigned long [2]),
    },
};

static __init int
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
