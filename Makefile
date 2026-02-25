# ld -T link.ld -o build/kernel.bin build/boot.o build/kernel.o build/calc.o build/keyboard.o build/io.o build/string.o build/screen.o

all:
	rm -rf build
	mkdir build
	nasm boot.asm -f elf32 -o build/boot.o
	gcc kernel.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -c -o build/kernel.o 
	gcc drivers/keyboard.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -c -o build/keyboard.o
	gcc drivers/io.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -c -o build/io.o
	gcc drivers/screen.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -c -o build/screen.o
	gcc lib/string.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -c -o build/string.o
	gcc apps/gui.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -c -o build/gui.o
	gcc apps/calc.c -m32 -O2 -fno-pie -fno-builtin -fomit-frame-pointer -c -o build/calc.o
	ld -T link.ld -o build/kernel.bin build/*.o
	cp build/kernel.bin iso/boot
	grub-mkrescue -o bomboclaat-os.iso iso

run:
	qemu-system-i386 -cdrom bomboclaat-os.iso -audiodev pa,id=speaker -machine pcspk-audiodev=speaker
