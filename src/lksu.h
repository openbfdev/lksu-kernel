/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _LKSU_KERNEL_H_
#define _LKSU_KERNEL_H_

#include <linux/types.h>
#include <linux/uuid.h>

#ifndef LKSU_SYSCALL_CTLKEY
# define LKSU_SYSCALL_CTLKEY 0x55aa00ff
#endif

#ifndef LKSU_LSM_ID
# define LKSU_LSM_ID 99
#endif

#define LKSU_TOKEN_LEN 36
#define LKSU_DEBUG 1

enum lksu_func {
    LKSU_ENABLE = 0,
    LKSU_DISABLE,
    LKSU_FLUSH,

    LKSU_GLOBAL_HIDDEN_ADD,
    LKSU_GLOBAL_HIDDEN_REMOVE,
    LKSU_GLOBAL_UID_ADD,
    LKSU_GLOBAL_UID_REMOVE,

    LKSU_TOKEN_ADD,
    LKSU_TOKEN_REMOVE,
    LKSU_FUNC_MAX_NR,
};

struct lksu_message {
    char token[LKSU_TOKEN_LEN];
    enum lksu_func func;

    union {
        /* LKSU_TOKAN_* */
        char token[LKSU_TOKEN_LEN];

        /* LKSU_GLOBAL_HIDDEN_* */
        const char *g_hidden;

        /* LKSU_GLOBAL_UID_* */
        __kernel_uid_t g_uid;
    } args;
};

#endif /* _LKSU_KERNEL_H_ */
