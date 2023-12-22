# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
#

make = make
linux ?= /lib/modules/$(shell uname -r)/build
src := $(shell pwd)/src

all:
	$(Q) $(make) -C $(linux) M=$(src) modules
PHONY += all

clean:
	$(Q) $(make) -C $(linux) M=$(src) clean
PHONY += clean

# Declare the contents of the PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)
