# BOMBOCLAAT-OS - simple x86_64 operating system
# Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
# SPDX-License-Identifier: GPL-3.0-or-later

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
        -g \
        -w
LDFLAGS = -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 --gc-sections -T link.ld

C_SOURCES = $(shell find . -path "./iso" -prune -o -path "./include" -prune -o -path "./bomboclaat-os" -prune -o -name "*.c" -print)
CPP_SOURCES = $(shell find . -path "./iso" -prune -o -path "./include" -prune -o -path "./bomboclaat-os" -prune -o -name "*.cpp" -print)
ASM_SOURCES = $(shell find . -path "./iso" -prune -o -path "./include" -prune -o -path "./bomboclaat-os" -prune -o -name "*.asm" -print)

C_OBJECTS = $(patsubst ./%, build/%.o, $(C_SOURCES))
CPP_OBJECTS = $(patsubst ./%, build/%.o, $(CPP_SOURCES))
ASM_OBJECTS = $(patsubst ./%, build/%.o, $(ASM_SOURCES))
ALL_OBJECTS = $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)

BUILD_NO := $(shell [ -f build_no.txt ] && cat build_no.txt || echo 0)
NEW_BUILD_NO := $(shell echo $$(($(BUILD_NO) + 1)))

INITRAMFS = initramfs.cpio

all: limine_download prepare $(ALL_OBJECTS) link $(INITRAMFS) iso_gen
	@echo "Done"

prepare:
	@echo $(NEW_BUILD_NO) > build_no.txt
	@mkdir -p build iso

limine_download:
	@if [ ! -d "limine-binary" ]; then \
	    echo "Downloading Limine binaries..."; \
	    curl -L -o limine.tar.gz https://github.com/Limine-Bootloader/Limine/releases/latest/download/limine-binary.tar.gz; \
	    mkdir -p limine-tmp; \
	    tar -xf limine.tar.gz -C limine-tmp; \
	    mv limine-tmp/* limine-binary; \
	    rm -rf limine-tmp limine.tar.gz; \
	    echo "Building limine utility..."; \
	    $(MAKE) -C limine-binary; \
	fi

build/%.c.o: %.c
	@mkdir -p "$(dir $@)"   
	@echo "  CC   $<"
	@gcc $(CFLAGS) -D BUILD_NUMBER=$(NEW_BUILD_NO) -c $< -o $@ 

build/%.cpp.o: %.cpp
	@mkdir -p "$(dir $@)"   
	@echo "  CC   $<"
	@g++ $(CFLAGS) -c $< -o $@ 

build/%.asm.o: %.asm
	@mkdir -p "$(dir $@)"   
	@echo "  AS   $<"
	@nasm -f elf64 $< -o $@

build_os:
	@$(MAKE) -C bomboclaat-os --no-print-directory

$(INITRAMFS): build_os
	@mv bomboclaat-os/initramfs.cpio $(INITRAMFS)

link:
	$(eval ALL_OBJ := $(shell find build -name "*.o"))
	$(eval BOOT_OBJ := $(shell find build -name "boot.asm.o"))
	$(eval OTHER_OBJ := $(filter-out $(BOOT_OBJ), $(ALL_OBJ)))
	@echo "  LD   build/bomboclaat"
	@ld $(LDFLAGS) -o build/bomboclaat $(BOOT_OBJ) $(OTHER_OBJ)

iso_gen:
	@echo "Generating ISO image..."
	@mkdir -p iso/boot/limine
	@mkdir -p iso/EFI/BOOT
	
	@cp build/bomboclaat iso/boot/
	@cp limine.conf iso/boot/limine/
	@cp initramfs.cpio iso/boot/
	
	@cp limine-binary/limine-bios.sys limine-binary/limine-bios-cd.bin \
	    limine-binary/limine-uefi-cd.bin iso/boot/limine/
	@cp limine-binary/BOOTX64.EFI iso/EFI/BOOT/
	@cp limine-binary/BOOTIA32.EFI iso/EFI/BOOT/
	
	@xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
	    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
	    -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
	    -efi-boot-part --efi-boot-image --protective-msdos-label \
	    iso -o bomboclaat-os.iso > /dev/null 2>&1
	    
	@./limine-binary/limine bios-install bomboclaat-os.iso > /dev/null 2>&1

run:
	@echo "Running in QEMU"
	@qemu-system-x86_64 \
	    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/x64/OVMF_CODE.4m.fd \
	    -drive if=pflash,format=raw,file=./OVMF_VARS.fd \
	    -cdrom bomboclaat-os.iso \
	    -hda disk.img \
	    -boot d \
	    -audiodev pipewire,id=speaker \
	    -machine pcspk-audiodev=speaker \
	    -m 2048M \
		-machine q35,acpi=on \
    	-device virtio-balloon \
	> /dev/null 2>&1

run-debug:
	@echo "Running in QEMU"
	@qemu-system-x86_64 \
	    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/x64/OVMF_CODE.4m.fd \
	    -drive if=pflash,format=raw,file=./OVMF_VARS.fd \
	    -cdrom bomboclaat-os.iso \
	    -hda disk.img \
	    -boot d \
	    -audiodev pipewire,id=speaker \
	    -machine pcspk-audiodev=speaker \
	    -m 1024M \
		-no-shutdown \
		-no-reboot \
		-d int,cpu_reset \
		-D qemu.log \
		-monitor stdio \
		-machine q35,acpi=on \
		-device virtio-balloon \
		-cpu Qemu64,+x2apic \
	> /dev/null 2>&1

disk-img:
	@qemu-img create -f raw disk.img 256M
	@mkfs.fat -F 32 disk.img
	@mkdir data
	@mkdir data/info
	@echo "Hello from data folder!" > data/hello.txt
	@mcopy -i disk.img -s data ::/
	@echo "Hello from the root directory!" > hello.txt
	@mcopy -i disk.img hello.txt ::hello.txt
	@rm -rf data
	@rm hello.txt

clean:
	@rm -rf build/ iso/ bomboclaat-os.iso
	@rm -f $(INITRAMFS)
	@$(MAKE) -C bomboclaat-os clean --no-print-directory

clean-all: clean
	@rm -rf limine-binary

.PHONY: all prepare build_os clean clean-all run disk-img limine_download
