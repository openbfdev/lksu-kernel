/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _LKSU_TABLES_H_
#define _LKSU_TABLES_H_

#include <linux/module.h>
#include <linux/types.h>

extern bool
lksu_table_gfile_check(const char *name);

extern bool
lksu_table_gdirent_check(const char *name);

extern int
lksu_table_gfile_add(const char *name);

extern int
lksu_table_gfile_remove(const char *name);

extern bool
lksu_table_guid_check(kuid_t kuid);

extern int
lksu_table_guid_add(kuid_t kuid);

extern int
lksu_table_guid_remove(kuid_t kuid);

extern void
lksu_table_flush(void);

extern int
lksu_tables_init(void);

extern void
lksu_tables_exit(void);

#endif /* _LKSU_TABLES_H_ */
