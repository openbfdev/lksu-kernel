/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#include <linux/lsm_hooks.h>

#define LSM_RET_DEFAULT(NAME) (NAME##_default)
#define DECLARE_LSM_RET_DEFAULT_void(DEFAULT, NAME)
#define DECLARE_LSM_RET_DEFAULT_int(DEFAULT, NAME) \
    static const int __maybe_unused LSM_RET_DEFAULT(NAME) = (DEFAULT);
#define LSM_HOOK(RET, DEFAULT, NAME, ...) \
    DECLARE_LSM_RET_DEFAULT_##RET(DEFAULT, NAME)

#include <linux/lsm_hook_defs.h>
#undef LSM_HOOK

static int
lsm_file_open(struct file *file)
{
    return hook_file_open(file);
}

static void
lsm_file_free(struct file *file)
{
    hook_file_free(file);
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
    LSM_HOOK_INIT(file_free_security, lsm_file_free),
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
