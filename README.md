# BOMBOCLAAT-OS 2.x

BOMBOCLAAT-OS is a simple x86_64 operating system with own kernel (*Mierdux*) written (mostly) in C. This version evolved evolved from BOMBOCLAAT-OS 1.x (see *legacy* branch) and is way more mature and advanced.

# Mierdux

Mierdux stands for mierda (which means shit in Spanish) and UNIX (because it's supposed to be UNIX-like). I named it like that because once I was writing the code, I got a bit mad (like always) and this beautful word came to my mind. As you can see Spanish is really worth learning.

# Features

<ol>
    <li>Framebuffer support</li>
    <li>x86_64 long mode</li>
    <li>Process management and scheduler</li>
    <li>TMPFS</li>
    <li>FAT32 driver (read-only for now)</li>
    <li>SYSCALL instruction support</li>
    <li>PMM and VMM</li>
    <li>External initramfs</li>
    <li>Separate kernel and user space</li>
</ol>

# Building and running

## Setup

Make sure you have these packages: `make`, `gcc`, `nasm`, `binutils`, `limine`, `xorriso`, `ovmf`. I **strongly** recommend to do everything on Linux, because on Windows it'll probably fuck up. I didn't test it on macOS, but on older Macs with Intel processors it should work without problems.

## Building

Clone the repo and go into cloned directory:
```sh
$ git clone https://github.com/bomboclaat954/BOMBOCLAAT-OS.git && cd BOMBOCLAAT-OS
```
Clean the files and build everything:
```sh
$ make clean && make -j$(nproc)
```
If everything goes right, you'll see `bomboclaat-os.iso` file in the main directory. <br>

## Running

If you want to run it on QEMU (make sure you have `qemu`, `qemu-system-x86`):
```sh
$ make run
```
You can also connect gdb debugger if you want:
```sh
$ make run-gdb
```
And in gdb:
```sh
(gdb) target remote localhost:1234
(gdb) continue
```
Alternatively, you can run it on VirtualBox.

# Hardware requirements

<ul>
    <li>x86_64 processor</li>
    <li>128 MB of RAM</li>
    <li>CPU supporting LAPIC timer</li>
</ul>

# Notes

1. If you followed versions 1.x, you may think that this project went backwards in development (because there's less commands), but actually it's the biggest progres that could happen. From a dumb, endless loop of stupid CLI it evolved into a real and (theoretically) usable kernel and OS. **This version is still in beta, so some things may not work properly or at all. If you found a bug, please report it to me.**
2. It's highly recommended to use BOMBOCLAAT-OS with UEFI; some things might not work on BIOS or errors may occur.
3. For understanding everything better, read `README.md` file in each folder.
4. If you looked into the commits you may noticed that I publish not working ones quite often. That's because I don't like when I have many uncommited changes and also I don't want the project to look dead. If something doesn't work, I'll probably fix it within 30 business days.

# Contributing

Writing an entire OS alone is hard, so any help would be really appreciated. If you have any suggestions or if you want to become a co-author, contact me and we'll discuss what can you do for the project. I need some people to help me with that.

# Contact

If you have any questions, you can text me on my [Telegram](https://t.me/bomboclaat954) or Discord (bomboclaat954).

# License

BOMBOCLAAT-OS is published as an open-source project under the GNU GPL v3 license.
