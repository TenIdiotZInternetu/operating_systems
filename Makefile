# SPDX-License-Identifier: Apache-2.0
# Copyright 2023 Charles University

-include config.mk

### Phony targets

.PHONY: all clean distclean kernel userspace cstyle check-cstyle check-config



### Default target

all: kernel userspace

kernel: check-config
	$(MAKE) -C kernel

userspace: check-config
	$(MAKE) -C userspace

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C userspace clean

distclean:
	$(MAKE) -C kernel distclean
	$(MAKE) -C userspace distclean
	rm -f config.mk

check-cstyle:
	./tools/check_cstyle.sh kernel/
	./tools/check_cstyle.sh userspace/

cstyle:
	find kernel/ -name '*.[ch]' -exec clang-format -style=file -i {} \;
	find userspace/ -name '*.[ch]' -exec clang-format -style=file -i {} \;


### Usability target

check-config:
    ifeq (,$(wildcard ./config.mk))
	$(error Missing 'config.mk'. Run 'configure.py' to create it)
    endif
