/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _LKSU_HOOKS_H_
#define _LKSU_HOOKS_H_

#include <linux/module.h>

//TODO
#include <../lksu.h>

extern int __init
lksu_hooks_init(void);

extern void __exit
lksu_hooks_exit(void);

typedef bool (*lksu_control_func_pt)(struct lksu_message *msg, int *retptr);

struct lksu_control_func {
    lksu_func func;
    lksu_control_func_pt handler;
};
#endif /* _LKSU_HOOKS_H_ */
