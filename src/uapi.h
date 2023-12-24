/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _LKSU_UAPI_H_
#define _LKSU_UAPI_H_

#include <linux/types.h>
#include <linux/uuid.h>

#ifndef LKSU_SYSCALL_CTLKEY
# define LKSU_SYSCALL_CTLKEY 0x55aa00ff
#endif

#define LKSU_TOKEN_LEN 36

enum lksu_func {
    LKSU_ENABLE = 0,
    LKSU_DISABLE,

    LKSU_GLOBAL_HIDDEN_ADD,
    LKSU_GLOBAL_HIDDEN_REMOVE,
    LKSU_GLOBAL_PROPERTY_ADD,
    LKSU_GLOBAL_PROPERTY_REMOVE,

    LKSU_TOKEN_ADD,
    LKSU_TOKEN_REMOVE,
    LKSU_FUNC_MAX_NR,
};

struct lksu_property {
    __kernel_pid_t pid;
    __kernel_uid_t uid;
    __kernel_gid_t gid;
};

struct lksu_message {
    char token[LKSU_TOKEN_LEN];
    enum lksu_func func;

    union {
        /* LKSU_TOKEN_* */
        char token[LKSU_TOKEN_LEN];
    } args;
};

#endif /* _LKSU_UAPI_H_ */
