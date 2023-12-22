/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#include <linux/kprobes.h>

static int
kprobe_keyset(struct kprobe *probe, struct pt_regs *regs)
{
	struct pt_regs *real_regs = (struct pt_regs *)PT_REGS_PARM1(regs);
    unsigned long args[4];
    int option;


    unsigned long arg2 = (unsigned long)PT_REGS_PARM2(real_regs);
    unsigned long arg2 = (unsigned long)PT_REGS_PARM2(real_regs);


    return 0;
}

static struct kprobe *
kprobe_hooks[] = {
    &(struct kprobe) {
        .symbol_name = "security_task_prctl",
        .pre_handler = kprobe_keyset,
    },
};

static int
hooks_kprobe_init(void)
{
    pr_notice("used kprobe function\n");
	return register_kprobes(kprobe_hooks, ARRAY_SIZE(kprobe_hooks));
}

static void
hooks_kprobe_exit(void)
{
	unregister_kprobes(kprobe_hooks, ARRAY_SIZE(kprobe_hooks));
}
