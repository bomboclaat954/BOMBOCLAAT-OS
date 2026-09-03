# bomboclaat-os

This directory contains the actual BOMBOCLAAT-OS code, the user-side operating system layer. It communicates with Mierdux Kernel through system calls. Files in this directory (but not in its subdirs) become ELF64 files in `/bin`, so you can use them as commands in the shell.

## bomboclaat-os/root

This is the initramfs template containing all of the commands.

## bomboclaat-os/lib

Here the essential libraries exist.

## bomboclaat-os/drivers

User-side drivers (not used yet).
