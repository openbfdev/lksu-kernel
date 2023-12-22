/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _LKSU_HOOKS_H_
#define _LKSU_HOOKS_H_

#include <linux/module.h>

extern int __init
lksu_hooks_init(void);

extern void __exit
lksu_hooks_exit(void);

#endif /* _LKSU_HOOKS_H_ */
