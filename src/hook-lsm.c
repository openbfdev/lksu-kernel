/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#include <linux/lsm_hooks.h>

static int
lsm_task_prctl(int option, unsigned long arg2, unsigned long arg3,
	       unsigned long arg4, unsigned long arg5)
{
    if (option == LKSU_SYSCALL_CTLKEY)
        return hooks_control((void __user *)arg2);
    return 0;
}

static struct security_hook_list
lsm_hooks[] = {
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
    /* Nothing */
}
