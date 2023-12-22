/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#define MODULE_NAME "lksu-hooks"
#define pr_fmt(fmt) MODULE_NAME ": " fmt

#include "hooks.h"
#include "token.h"
#include "uapi.h"
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/printk.h>

static int
hooks_control(struct lksu_message __user *message)
{
	struct lksu_message msg;
	unsigned long length;
	bool verify;

	if (!message)
		goto failed;

	length = copy_from_user(&msg, message, sizeof(msg));
	if (length != sizeof(msg))
		goto failed;

	verify = lksu_token_verify(msg.token);
	if (verify)
		goto failed;

	printk("pass\n");

failed:
	return 0;
}

#ifdef MODULE
# include "hook-kprobe.c"
#else
# include "hook-lsm.c"
#endif

int __init
lksu_hooks_init(void)
{
#ifdef MODULE
	return hooks_kprobe_init();
#else
	return hooks_lsm_init();
#endif
}

void __exit
lksu_hooks_exit(void)
{
#ifdef MODULE
	hooks_kprobe_exit();
#else
	hooks_lsm_exit();
#endif
}
