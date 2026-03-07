all:
	rm -rf build
	rm -rf iso
	mkdir build
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	nasm boot.asm -f elf32 -o build/boot.o

	gcc kernel.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -ffreestanding -c -o build/kernel.o -O0
	gcc drivers/keyboard.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -ffreestanding -c -o build/keyboard.o -O0
	gcc drivers/io.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -ffreestanding -c -o build/io.o -O0
	gcc drivers/screen.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -ffreestanding -c -o build/screen.o -O0
	gcc lib/string.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -ffreestanding -c -o build/string.o -O0
	gcc apps/gui.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -ffreestanding -c -o build/gui.o -O0
	gcc apps/calc.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -ffreestanding -c -o build/calc.o -O0

	ld -m elf_i386 -T link.ld -o build/kernel.bin build/*.o
	cp grub.cfg iso/boot/grub
	cp build/kernel.bin iso/boot
	grub-mkrescue -o bomboclaat-os.iso iso

run:
	qemu-system-i386 -cdrom bomboclaat-os.iso -audiodev pa,id=speaker -machine pcspk-audiodev=speaker
