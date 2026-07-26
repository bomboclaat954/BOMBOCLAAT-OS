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
// I have no clue if all these includes are needed but I'm too scared to touch it, if it works, don't touch it
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
#include <drivers/screen.h>
#include <tasks/tasks.h>
#include <fs/vfs.h>
#include <stddef.h>
#include <errno.h>

#define TEMP_MAP_ADDR 0xFFFFFFFFF0000000
#define IA32_EFER 0xC0000080
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084
#define EFER_SCE (1ULL << 0)

extern void syscall_entry(void);

static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint32_t msr, uint64_t value)
{
    uint32_t low = (uint32_t)value;
    uint32_t high = (uint32_t)(value >> 32);
    asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

void init_syscall(uint16_t kernel_cs, uint16_t user_cs_base)
{
    uint64_t efer = rdmsr(IA32_EFER);
    wrmsr(IA32_EFER, efer | EFER_SCE);
    wrmsr(IA32_LSTAR, (uint64_t)syscall_entry);

    uint64_t star = ((uint64_t)kernel_cs << 32) | ((uint64_t)user_cs_base << 48);
    wrmsr(IA32_STAR, star);
    wrmsr(IA32_FMASK, 0x200);
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
    case 2: // create a new process (task_t) for an executable file
    {
        char *name = (char *)r->rdi;
        char **argv = (char **)r->rsi;
        int argc = (int)r->rdx;

        uint64_t size = 0;
        int fd = vfs_open(name, 0, &size);
        if (fd < 0)
            return 0;

        void *file = kmalloc(size + 1);
        if (file == NULL)
        {
            vfs_close(fd);
            return 0;
        }

        int64_t read_bytes = vfs_read(fd, file, size);
        vfs_close(fd);
        if (read_bytes < 0)
            return 0;

        int frames = (size + PAGE_SIZE - 1) >> 12;
        task_t *new_task = task_create(file, current_task->pid, name, argc, argv, frames);
        if (new_task == NULL)
            return 0;

        current_task->state = TASK_BLOCKED;
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
    case 4:
    {
        // TODO: get rid of this and write to /dev/kbd
        char *buf = (char *)r->rdi;
        input(buf);
        r->rax = 1;
        return (uint64_t)r;
    }
    case 5: // cls
    {
        cls();
        r->rax = 1;
        return (uint64_t)r;
    }
    case 6: // get framebuffer info (RDI = 0 - pitch, RDI = 1 - height, RDI = 2 - width)
    {
        int x = (int)r->rdi;
        if (x == 0)
        {
            extern uint64_t fbf_pitch;
            r->rax = fbf_pitch;
        }
        else if (x == 1)
        {
            extern uint64_t fbf_height;
            r->rax = fbf_height;
        }
        else if (x == 2)
        {
            extern uint64_t fbf_width;
            r->rax = fbf_width;
        }
        return r->rax;
    }
    case 7: // uname
    {
        int type = (int)r->rdi;
        char *ret_buf = (char *)r->rsi;

        if (type == 0)
            // strcpy(UNAME[0], ret_buf);
            memcpy(ret_buf, UNAME[0], strlen(UNAME[0]));
        else if (type == 1)
            // strcpy(UNAME[1], ret_buf);
            memcpy(ret_buf, UNAME[1], strlen(UNAME[1]));
        else if (type == 2)
            // strcpy(UNAME[2], ret_buf);
            memcpy(ret_buf, UNAME[2], strlen(UNAME[2]));
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
            vmm_map_page(kernel_pml4_virt, (uintptr_t)init_heap_current, (uintptr_t)frame, VMM_PRESENT | VMM_WRITE | VMM_USER);
        }
        return (uint64_t)previous_heap_end;
    }
    case 10: // file open
    {
        char *path = (char *)r->rdi;
        int flags = (int)r->rsi;
        if (path[0] != '/')
            return -1;
        uint64_t size = 0;
        int x = vfs_open(path, flags, &size);
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
        r->rax = ENOSYS;
        return (uint64_t)r;
    }
    }
}
