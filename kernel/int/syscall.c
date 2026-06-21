/*
 * BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/*
    BOMBOCLAAT-OS SYSCALLS (int 0x80):
        * 1 - kprintf
        * 2 - execute
        * 3 - create new process
            * RDI = PROCNAME
            * RSI = PROCINFO
        * 4 - char in
        * 5 - disk operations:
            * RDI = 0 - read (RSI = *buf)
            * RDI = 1 - write (RSI = *buf)
        * 6 - ramfs:
            * RDI = 0 - MKDIR (RSI - ramfs_dir_t *dir)
            * RDI = 1 - MKFILE (RSI - ramfs_file_t *file)
        * 7 - uname
            * RDI - TYPE (1 - kernel)
            * RSI - return buffer
        * 8 - reboot or shutdown
            * RDI = 0 - REBOOT
            * RDI = 1 - SHUTDOWN
        * 9 - SBRK
    Syscall registers:
        * RAX - syscall number
        Arguments:
            * RDI - 1st
            * RSI - 2nd
            * RDX - 3rd
            * R10 - 4th
            * R8  - 5th
            * R9  - 6th
*/

#include <int/int.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/initramfs.h>
#include <tasks/loader.h>
#include <lib/string.h>
#include <fs/ramfs.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <drivers/io.h>
#include <tasks/tasks.h>
#include <stddef.h>

#define TEMP_MAP_ADDR 0xFFFFFFFFF0000000
extern void enter_user_mode(uintptr_t entry_point, uintptr_t user_stack);

void reboot()
{
    unsigned char good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
}

void shutdown()
{
    outw(0xB004, 0x2000); // QEMU
    outw(0x604, 0x2000);  // QEMU / Bochs
    outw(0x4004, 0x3400); // VirtualBox
    asm volatile("cli");
    asm volatile("hlt");
}

void syscall_send(uint64_t nsyscall, const char *args)
{
    asm volatile(
        "int $0x80"
        :
        : "a"(nsyscall), "D"(args)
        : "memory");
}

uint64_t syscall_handler(context_t *r)
{
    extern task_t *current_task;
    extern task_t *task_list_head;
    switch (r->rax)
    {
    case 1:
        kprintf((const char *)r->rdi);
        return 0;
    case 2:
        char *name = (char *)r->rdi;
        int current_pid = current_task->pid;
        extern void *initramfs_base;
        uint64_t size = 0;
        void *file = initramfs_find_file(initramfs_base, name, &size);
        if (file == NULL)
            return 0;

        int frames = (size + PAGE_SIZE - 1) >> 12;
        task_t *new_task = task_create(file, current_task->pid, name, frames);
        if (new_task == NULL)
            return 0;

        new_task->parent_pid = current_pid;
        new_task->next = current_task;
        current_task->state = TASK_BLOCKED;
        new_task->state = TASK_READY;
        r->rax = 1;
        schedule(r);
        return (uint64_t)r;
    case 3:
        task_exit(r);
        while (1)
            asm volatile("hlt");
    case 4:
        char *buf = (char *)r->rdi;
        input(buf);
        r->rax = 1;
        return (uint64_t)r;
    case 5:
        r->rax = 1;
        return (uint64_t)r;
    case 6:
        r->rax = 1;
        return (uint64_t)r;
    case 7:
        int type = (int)r->rdi;
        char *ret_buf = (char *)r->rsi;
        if (type == 0)
            strcpy(UNAME[0], ret_buf);
        else if (type == 1)
            strcpy(UNAME[1], ret_buf);
        else if (type == 2)
            strcpy(UNAME[2], ret_buf);
        r->rax = 1;
        return (uint64_t)r;
    case 8:
        int x = (int)r->rdi;
        if (x == 0)
            reboot();
        else if (x == 1)
            shutdown();
        r->rax = 1;
        return (uint64_t)r;
    case 9:
        extern vmm_table_t *kernel_pml4_virt;
        size_t increment = (size_t)r->rdi;
        uint8_t *previous_heap_end = init_heap_current;
        init_heap_current += increment;
        for (int i = 0; i < (increment / PAGE_SIZE) + 1; i++)
        {
            init_heap_current += i;
            void *frame = pmm_alloc_frame();
            vmm_map_page(kernel_pml4_virt, (uintptr_t)init_heap_current, (uintptr_t)frame, VMM_PRESENT | VMM_WRITE); // | VMM_USER
        }
        return (uint64_t)previous_heap_end;
    default:
        kprintf("Unknown syscall\n");
        r->rax = 1;
        return (uint64_t)r;
    }
}
