# core

This folder contains the actual Mierdux kernel code, the core of this whole operating system.

## core/arch

Architecture-specific low-level code (e.g. CPU context switch).

## core/drivers

Basic kernel-side drivers for screen, keyboard and ATA (not used, to be removed)

## core/memory

Memory management, paging. PMM stands for physical memory manager (mm), VMM is virtual mm. `memtools.c` contains functions that normally exist in `string.h`, that's a historical mistake form 1.x that I'm too lazy to fix.

## core/syscall

`SYSCALL` instruction support, system call dispatcher and basic logic.

## core/tasks

Process management, scheduler, etc. The worst part to write imo, gave me a few headaches and many thoughts to delete this fucking project from the Internet.
