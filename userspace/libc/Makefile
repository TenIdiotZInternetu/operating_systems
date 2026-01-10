# SPDX-License-Identifier: Apache-2.0
# Copyright 2023 Charles University

### Compiler, assembler and linker options
#
# All the options are described in detail in the GCC
# and binutils documentation. A brief description of
# some of the specific ones:
#
# -O2                         ... basic optimizations
# -ffreestanding              ... a compilation without standard library and main()
# -nostdlib                   ... do not look for standard library in system directories
# -nostdinc                   ... do not look for standard header files in system directories
# -fno-pic                    ... do not generate position independent code using $gp
# -fno-builtin                ... do not recognize built-in functions without __builtin_ prefix
# -mstrict-align              ... force strict alignment, because MSIM requires it
# -mno-riscv-attribute        ... do not emit RISC-V attibute to record extra information into ELF objects
# -msmall-data-limit=0        ... do not put small static data into a special section
# -march=rv32g                ... enable only basic extensions
#


-include ../../config.mk

CCFLAGS = -ffile-prefix-map=$(SRC_BASE)= -I$(SRC_BASE)include -O2 -msmall-data-limit=0 -mstrict-align -fno-pic -fno-builtin -ffreestanding -nostdlib -nostdinc -mno-riscv-attribute -pipe -Wall -Wextra -Werror -Wno-unused-parameter -Wmissing-prototypes -g3 -std=c11 -march=rv32g
ASFLAGS = -I$(SRC_BASE)include -msmall-data-limit=0 -mstrict-align -fno-pic -fno-builtin -ffreestanding -nostdlib -nostdinc -mno-riscv-attribute -pipe -Wall -Wextra -Werror -Wno-unused-parameter -Wmissing-prototypes -g3 -std=c11 -I. -D__ASM__ -march=rv32g
LDFLAGS = -G 0 -static -g -Llibc

### Libc sources and objects

LIBC_SOURCES = \
	src/entry.S \
	src/exit.c \
	src/heap.c \
	src/main.c \
	src/print.c \
	src/proc.c \
	src/runtime.c \
	src/stackspace.S

LIBC_OBJECTS := $(addsuffix .o,$(basename $(LIBC_SOURCES)))

DEPS := $(addsuffix .dep,$(basename $(LIBC_SOURCES)))

.PHONY: .FORCE all clean distclean dir-tree

all: libc.a

dir-tree:
	for i in $(LIBC_OBJECTS); do mkdir -p `dirname $$i`; done

distclean: clean
	rm -f libc.a

clean:
	find . \( -name '*.o' -o -name '*.dep' \) -exec rm -f \{\} \;

-include $(DEPS)

### Library assembly

libc.a: $(LIBC_OBJECTS)
	$(AR) rc $@ $(LIBC_OBJECTS)

### Default patterns

%.o: %.c | dir-tree
	$(CC) $(CCFLAGS) $(SHARED_EXTRA_CFLAGS) -MD -MF $*.dep -c -o $@ $<

%.o: %.S | dir-tree
	$(CC) $(ASFLAGS) -c -o $@ $<
