CFLAGS = -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -ffreestanding -c -Iinclude

all:
	rm -rf build
	rm -rf iso
	mkdir build
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	nasm boot.asm -f elf32 -o build/boot.o
	nasm int/isr.asm -f elf32 -o build/isr_asm.o

	gcc kernel.c $(CFLAGS) -o build/kernel.o
	gcc drivers/keyboard.c $(CFLAGS) -o build/keyboard.o
	gcc drivers/io.c $(CFLAGS) -o build/io.o
	gcc drivers/screen.c $(CFLAGS) -o build/screen.o
	gcc drivers/disk.c $(CFLAGS) -o build/disk.o
	gcc lib/string.c $(CFLAGS) -o build/string.o
	gcc lib/math.c $(CFLAGS) -o build/math.o
	gcc lib/rand.c $(CFLAGS) -o build/rand.o
	gcc apps/calc.c $(CFLAGS) -o build/calc.o
	gcc apps/diskman.c $(CFLAGS) -o build/diskman.o
	gcc memory/ram.c $(CFLAGS) -o build/ram.o
	gcc int/idt.c $(CFLAGS) -o build/idt.o
	gcc int/isr.c $(CFLAGS) -o build/isr_c.o
	gcc int/pic.c $(CFLAGS) -o build/pic.o
	gcc int/irq.c $(CFLAGS) -o build/irq.o
	gcc int/pit.c $(CFLAGS) -o build/pit.o
	gcc music/music.c $(CFLAGS) -o build/music.o

	ld -m elf_i386 -T link.ld -o build/kernel.bin build/*.o
	cp grub.cfg iso/boot/grub
	cp build/kernel.bin iso/boot
	grub-mkrescue -o bomboclaat-os.iso iso

run:
	qemu-system-i386 -cdrom bomboclaat-os.iso -hda disk.img -boot d -audiodev pa,id=speaker -machine pcspk-audiodev=speaker

disk-img:
	qemu-img create -f raw disk.img 256M
	mkfs.fat -F 32 disk.img
