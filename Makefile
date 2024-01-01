# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
#

make = make
src := $(shell pwd)/src
linux ?= /lib/modules/$(shell uname -r)/build
prefix ?= /usr

all:
	$(Q) $(make) -C $(linux) M=$(src) CONFIG_LKSU_MODULE=y modules
PHONY += all

clean:
	$(Q) $(make) -C $(linux) M=$(src) clean
PHONY += clean

install:
	$(Q) install -Dpm 644 src/lksu.h ${prefix}/include/lksu/kernel.h
PHONY += install

uninstall:
	$(Q) rm -rf ${prefix}/include/lksu/kernel.h
PHONY += uninstall

# Declare the contents of the PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)
