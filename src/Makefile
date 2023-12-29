# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
#

ifdef CONFIG_LKSU_MODULE
CONFIG_LKSU := m
ccflags-y += -DCONFIG_LKSU_HOOK_LIVEPATCH
endif

obj-$(CONFIG_LKSU) := lksu.o
lksu-y += hidden.o
lksu-y += hooks.o
lksu-y += main.o
lksu-y += tables.o
lksu-y += token.o
