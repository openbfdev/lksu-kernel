/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _LKSU_PROCFS_H_
#define _LKSU_PROCFS_H_

#include <linux/module.h>
#include <linux/types.h>

extern int __init
lksu_procfs_init(void);

extern void
lksu_procfs_exit(void);

#endif /* _LKSU_PROCFS_H_ */
