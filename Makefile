# BOMBOCLAAT-OS MAKEFILE v2.2
# I switched to Arch Linux from Ubuntu and there were some issues with build and QEMU but I fixed it

CFLAGS = -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -ffreestanding -Iinclude -march=i386
LDFLAGS = -m elf_i386 -T link.ld --no-warn-rwx-segments

C_SOURCES = $(shell find . -path "./iso" -prune -o -path "./include" -prune -o -name "*.c" -print)
CPP_SOURCES = $(shell find . -path "./iso" -prune -o -path "./include" -prune -o -name "*.cpp" -print)
ASM_SOURCES = $(shell find . -path "./iso" -prune -o -path "./include" -prune -o -name "*.asm" -print)

C_OBJECTS = $(patsubst ./%, build/%.o, $(C_SOURCES))
CPP_OBJECTS = $(patsubst ./%, build/%.o, $(CPP_SOURCES))
ASM_OBJECTS = $(patsubst ./%, build/%.o, $(ASM_SOURCES))
ALL_OBJECTS = $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)

BUILD_NO := $(shell [ -f build_no.txt ] && cat build_no.txt || echo 0)
COMMIT_NO := $(shell git rev-parse --short HEAD)
NEW_BUILD_NO := $(shell echo $$(($(BUILD_NO) + 1)))

all: prepare $(ALL_OBJECTS) link iso_gen
	@echo "Done"

prepare:
	@echo "╔════════════════════════╗"
	@echo "║ BOMBOCLAAT-OS BUILD v2 ║"
	@echo "╚════════════════════════╝"
	@echo "Build no: $(NEW_BUILD_NO)"
	@echo $(NEW_BUILD_NO) > build_no.txt
	@rm -rf build iso
	@mkdir -p build iso/boot/grub
	@find . -path "./iso" -prune -o -path "./include" -prune -o -path "./build" -prune -o -type d -not -name "." -exec mkdir -p build/{} \;

font:
	@echo "Parsing font"
	@python3 fonts.py bombofont.psf
	@mv font.h include/fonts/bombofont.h

build/%.c.o: %.c
	@echo "  GCC  $<"
	@gcc $(CFLAGS) -D BUILD_NUMBER=$(NEW_BUILD_NO) -D COMMIT_NUMBER=0x$(COMMIT_NO) -c $< -o $@ 

build/%.cpp.o: %.cpp
	@echo "  G++  $<"
	@g++ $(CFLAGS) -c $< -o $@ 

build/%.asm.o: %.asm
	@echo "  ASM  $<"
	@nasm -f elf32 $< -o $@

link:
	$(eval ALL_OBJ := $(shell find build -name "*.o"))
	$(eval BOOT_OBJ := $(shell find build -name "boot.asm.o"))
	$(eval OTHER_OBJ := $(filter-out $(BOOT_OBJ), $(ALL_OBJ)))
	@ld $(LDFLAGS) -o build/kernel.bin $(BOOT_OBJ) $(OTHER_OBJ)

iso_gen:
	@cp grub.cfg iso/boot/grub
	@cp build/kernel.bin iso/boot
	@grub-mkrescue -o bomboclaat-os.iso iso > /dev/null 2>&1

run:
	@echo "Running in QEMU"
	@qemu-system-i386 \
	-cdrom bomboclaat-os.iso \
	-hda disk.img \
	-boot d \
	-audiodev pipewire,id=speaker \
	-machine pcspk-audiodev=speaker \
	> /dev/null 2>&1

run-debug:
	qemu-system-i386 \
    -cdrom bomboclaat-os.iso \
    -hda disk.img \
    -boot d \
    -d int,cpu_reset \
    -no-reboot \
    -no-shutdown

disk-img:
	@qemu-img create -f raw disk.img 256M
	@mkfs.fat -F 32 disk.img
	@mkdir data
	@echo "Hello from data folder!" > data/hello.txt
	@mcopy -i disk.img -s data ::/
	@echo "Hello from the root directory!" > hello.txt
	@mcopy -i disk.img hello.txt ::hello.txt
	@rm -rf data
	@rm hello.txt
