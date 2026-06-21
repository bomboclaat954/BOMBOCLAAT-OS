# BOMBOCLAAT-OS - simple x86_64 operating system
# Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
# SPDX-License-Identifier: GPL-3.0-or-later

VER="BOMBOCLAAT-OS 2.0-beta1"

CFLAGS =-m64 \
        -O2 \
        -pipe \
        -ffreestanding \
        -fno-stack-protector \
        -fno-stack-check \
        -fno-pie \
        -fno-PIC \
        -mno-80387 \
        -mno-mmx \
        -mno-red-zone \
        -mcmodel=kernel \
        -Iinclude \
        -march=x86-64 \
        -g
LDFLAGS = -nostdlib --entry=_start -Ttext 0x400000

C_SOURCES   = $(shell find . -path "./iso" -prune -o -path "./include" -prune -o -name "*.c" -print)
CPP_SOURCES = $(shell find . -path "./iso" -prune -o -path "./include" -prune -o -name "*.cpp" -print)
ASM_SOURCES = $(shell find . -path "./iso" -prune -o -path "./include" -prune -o -name "*.asm" -print)

C_OBJECTS   = $(patsubst ./%, build/%.o, $(C_SOURCES))
CPP_OBJECTS = $(patsubst ./%, build/%.o, $(CPP_SOURCES))
ASM_OBJECTS = $(patsubst ./%, build/%.o, $(ASM_SOURCES))
ALL_OBJECTS = $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)

all: prepare $(ALL_OBJECTS) link mkramfs

prepare:
	@rm -rf build
	@mkdir -p build

build/%.c.o: %.c
	@mkdir -p "$(dir $@)"
	@echo "  CC   $<"
	@gcc $(CFLAGS) -D OSVER='$(VER)' -c $< -o $@

build/%.cpp.o: %.cpp
	@mkdir -p "$(dir $@)"
	@echo "  CC   $<"
	@g++ $(CFLAGS) -c $< -o $@

build/%.asm.o: %.asm
	@mkdir -p "$(dir $@)"
	@echo "  AS   $<"
	@nasm -f elf64 $< -o $@

link:
	$(eval ALL_OBJS := $(wildcard build/*.o))
	$(eval ENTRY_OBJ := build/entry.asm.o)
	$(eval APP_OBJS := $(filter-out $(ENTRY_OBJ),$(ALL_OBJS)))
	$(eval LIB_OBJS := $(wildcard build/lib/*.o))

	@for obj in $(APP_OBJS); do \
		name=$$(basename $$obj .c.o); \
		echo "  LD   build/$$name.elf"; \
		ld $(LDFLAGS) -o build/$$name.elf $(ENTRY_OBJ) $$obj $(LIB_OBJS) $(CMD_OBJS); \
	done

mkramfs:
	$(eval ELF_OBJS := $(wildcard build/*.elf))
	@for obj in $(ELF_OBJS); do \
		name=$$(basename $$obj .elf); \
		cp build/$$name.elf root/bin/$$name; \
	done
	@cd root && find . -mindepth 1 -printf '%P\n' | cpio -o -H newc > ../initramfs.cpio

clean:
	@rm -rf build
	@rm -f initramfs.img

.PHONY: clean prepare all link mkramfs
