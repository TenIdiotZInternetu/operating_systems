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


-include ../config.mk

CCFLAGS = -ffile-prefix-map=$(SRC_BASE)= -I$(SRC_BASE)libc/include -O2 -msmall-data-limit=0 -mstrict-align -fno-pic -fno-builtin -ffreestanding -nostdlib -nostdinc -mno-riscv-attribute -pipe -Wall -Wextra -Werror -Wno-unused-parameter -Wmissing-prototypes -g3 -std=c11 -march=rv32g
ASFLAGS = -I$(SRC_BASE)libc/include -msmall-data-limit=0 -mstrict-align -fno-pic -fno-builtin -ffreestanding -nostdlib -nostdinc -mno-riscv-attribute -pipe -Wall -Wextra -Werror -Wno-unused-parameter -Wmissing-prototypes -g3 -std=c11 -I. -D__ASM__ -march=rv32g
LDFLAGS = -G 0 -static -g -Llibc
LDFLAGS_LIBS = -lc

### Userspace linker script

LINKER_SCRIPT = $(SRC_BASE)app.lds

EXAMPLE_APP_SOURCES = example/main.c

ifeq ($(USERSPACE_TEST_SOURCES),)
	APP_SOURCES := $(EXAMPLE_APP_SOURCES)
else
	APP_SOURCES := $(USERSPACE_TEST_SOURCES)
endif
	


### Userspace application objects

APP_OBJECTS := $(addsuffix .o,$(basename $(APP_SOURCES)))
APP_RAW = app.raw
APP_MAP = app.map
APP_BINARY = app.bin
APP_DISASM = app.disasm

LIB_DEPS = libc/libc.a

DEPS := $(addsuffix .dep,$(basename $(APP_SOURCES)))

.PHONY: .FORCE all clean distclean dir-tree

all: $(APP_BINARY) $(APP_DISASM)

dir-tree:
	for i in $(APP_OBJECTS); do mkdir -p `dirname $$i`; done

distclean: clean
	$(MAKE) -C libc distclean
	rm -f $(APP_BINARY) $(APP_BINARY)

clean:
	$(MAKE) -C libc clean
	rm -f $(APP_DISASM) $(APP_MAP) $(APP_RAW)
	find . \( -name '*.o' -o -name '*.dep' \) -exec rm -f \{\} \;

-include $(DEPS)

%.disasm: %.raw
	$(OBJDUMP) -d -S $< > $@
	
### Libraries

libc/libc.a: .FORCE
	$(MAKE) -C libc all

### Binary images
#
# The binary files are made in two stages. First, ELF files are
# created from the object files. Next, the binary files are
# created from the ELF files. The reasons for this are:
#
#  - The ELF files contain debugging information.
#
#  - Linking directly to a binary format does
#    not produce errors on missing symbols.
#

%.bin: %.raw
	$(OBJCOPY) -O binary $< $@

$(APP_RAW): $(LINKER_SCRIPT) $(APP_OBJECTS) $(LIB_DEPS)
	$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) -Map $(APP_MAP) -o $@ $(APP_OBJECTS) $(LDFLAGS_LIBS)

### Default patterns

%.o: %.c | dir-tree
	$(CC) $(CCFLAGS) $(SHARED_EXTRA_CFLAGS) -MD -MF $*.dep -c -o $@ $<

%.o: %.S | dir-tree
	$(CC) $(ASFLAGS) -c -o $@ $<
