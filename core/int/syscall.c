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

#include <int/int.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/initramfs.h>
#include <tasks/loader.h>
#include <lib/string.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <drivers/io.h>
#include <drivers/acpi.h>
#include <tasks/tasks.h>
#include <fs/vfs.h>
#include <stddef.h>

#define TEMP_MAP_ADDR 0xFFFFFFFFF0000000

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
    case 1: // printf
    {
        kprintf((const char *)r->rdi);
        return 0;
    }
    case 2: // new task
    {
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
    }
    case 3: // task exit
    {
        task_exit(r);
        while (1)
            asm volatile("hlt");
    }
    case 4: // TODO: CHANGE THIS
    {
        char *buf = (char *)r->rdi;
        input(buf);
        r->rax = 1;
        return (uint64_t)r;
    }
    case 5:
    {
        r->rax = 1;
        return (uint64_t)r;
    }
    case 6:
    {
        r->rax = 1;
        return (uint64_t)r;
    }
    case 7: // uname
    {
        int type = (int)r->rdi;
        char *ret_buf = (char *)r->rsi;
        if (type == 0)
            strcpy(UNAME[0], ret_buf);
        else if (type == 1)
            strcpy(UNAME[1], ret_buf);
        else if (type == 2)
            strcpy(UNAME[2], ret_buf);
        else if (type == 3)
            strcpy(UNAME[3], ret_buf);
        else if (type == 4)
        {
            uint64_t *out_ptr = (uint64_t *)ret_buf;
            *out_ptr = get_free_frames();
        }
        else if (type == 4)
        {
            uint64_t *out_ptr = (uint64_t *)ret_buf;
            *out_ptr = get_total_frames();
        }
        r->rax = 1;
        return (uint64_t)r;
    }
    case 8: // reboot / shutdown
    {
        int x = (int)r->rdi;
        if (x == 0)
            acpi_reboot();
        else if (x == 1)
            acpi_shutdown();
        r->rax = 1;
        return (uint64_t)r;
    }
    case 9: // malloc
    {
        extern vmm_table_t *kernel_pml4_virt;
        size_t increment = (size_t)r->rdi;
        uint8_t *previous_heap_end = init_heap_current;
        init_heap_current += increment;
        for (int i = 0; i < (increment / PAGE_SIZE) + 1; i++)
        {
            init_heap_current += i;
            void *frame = pmm_alloc_frame();
            vmm_map_page(kernel_pml4_virt, (uintptr_t)init_heap_current, (uintptr_t)frame, VMM_PRESENT | VMM_WRITE);
        }
        return (uint64_t)previous_heap_end;
    }
    case 10: // file open
    {
        char *path = (char *)r->rdi;
        int flags = (int)r->rsi;
        int x = vfs_open(path, flags);

        if (x == -1)
        {
            extern vfs_inode_t *root_inode;
            vfs_mkfile(root_inode, path, 0);
            x = vfs_open(path, flags);
        }

        return x;
    }
    case 11: // file read
    {
        int fd = (int)r->rdi;
        uint64_t size = (uint64_t)r->rsi;
        void *buf = (void *)r->rdx;
        return vfs_read(fd, buf, size);
    }
    case 12: // file write
    {
        int fd = (int)r->rdi;
        uint64_t size = (uint64_t)r->rsi;
        void *buf = (void *)r->rdx;
        return vfs_write(fd, buf, size);
    }
    case 13: // file close
    {
        int fd = (int)r->rdi;
        return vfs_close(fd);
    }
    default:
    {
        kprintf("Unknown syscall\n");
        r->rax = 1;
        return (uint64_t)r;
    }
    }
}
