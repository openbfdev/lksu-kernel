/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#include <linux/kprobes.h>

static struct kprobe *
kprobe_hooks[] = {

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
