/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _LKSU_HIDDEN_H_
#define _LKSU_HIDDEN_H_

#include <linux/module.h>
#include <linux/fs.h>

extern int
lksu_hidden_dirent(struct file *file);

extern int
lksu_hidden_file(struct file *file, bool *hidden);

extern int
lksu_hidden_path(const struct path *path, bool *hidden);

extern int
lksu_hidden_inode(struct inode *inode, bool *hidden);

extern int
lksu_hidden_init(void);

extern void
lksu_hidden_exit(void);

#endif /* _LKSU_HIDDEN_H_ */
