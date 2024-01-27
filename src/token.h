/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _LKSU_TOKEN_H_
#define _LKSU_TOKEN_H_

#include <linux/uuid.h>

extern bool
lksu_token_verify(const char *token);

extern int
lksu_token_add(const char *token);

extern int
lksu_token_remove(const char *token);

extern void
lksu_token_flush(void);

extern int
lksu_token_init(void);

extern void
lksu_token_exit(void);

#endif /* _LKSU_TOKEN_H_ */
