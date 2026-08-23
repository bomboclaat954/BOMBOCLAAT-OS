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
// I have no clue if all these includes are needed but I'm too scared to remove them. If it works, don't touch it.
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
#include <memory/userspace.h>
#include <drivers/io.h>
#include <drivers/acpi.h>
#include <drivers/screen.h>
#include <tasks/tasks.h>
#include <tasks/exec.h>
#include <fs/vfs.h>
#include <stddef.h>
#include <errno.h>

// TODO: get rid of that old INT 0x80 and migrate to SYSCALL.
// TODO: UNIX-like syscalls: fork, execve, exit, etc.

/*
    Recently I came up with an interesting idea on managing syscalls.
    What if instead of using the standard way like INT 0x80 or SYSCALL
    we used JSON sent to a specific address readable to both user and
    kernel space and then simply send an interrupt?
    For example syscall 2 (look below in int128_handler switch case 2):
    {
        "syscall_num": 2,
        "data": {
            "path": "/bin/shell",
            "argv_ptr": 0x...,
            "argc": 3
        },
        "caller_pid": 8
    }
    Then the code hits INT 0x80
    And the kernel parses it and does its job. It may take a bit longer
    than now but I think it would be way more human readable and easier to
    understand. If you see this, tell me what do you think, my tg is in readme.
*/
// That ain't working... (see core/syscall/syscall.c)

#define TEMP_MAP_ADDR 0xFFFFFFFFF0000000

uint64_t int128_handler(context_t *r)
{
    extern task_t *current_task;
    extern task_t *task_list_head;
    switch (r->rax)
    {
    case 1: // printf
    {
        kprintf((const char *)r->rdi); // A professional OS developer would kill me for that XDD
        return 0;
    }
    case 2: // exec (old, stupid)
    {
        char *path = (char *)r->rdi;
        char **argv_ptr = (char **)r->rsi;
        int argc = (int)r->rdx;

        char *argv[argc];
        for (int i = 0; i < argc; i++)
        {
            size_t len = strlen(argv_ptr[i]);
            argv[i] = (char *)kmalloc(len + 1);
            memcpy(argv[i], argv_ptr[i], len + 1);
        }

        uint64_t size = 0;
        int fd = vfs_open(path, 0, &size, current_task->fd_table);
        if (fd < 0)
            return 1;

        void *file = kmalloc(size + 1);
        if (file == NULL)
        {
            vfs_close(fd, current_task->fd_table);
            return 2;
        }

        int64_t read_bytes = vfs_read(fd, file, size);
        vfs_close(fd, current_task->fd_table);
        if (read_bytes < 0)
            return 3;

        int frames = (size + PAGE_SIZE - 1) >> 12;
        task_t *new_task = task_create(file, current_task->pid, path, argc, argv, frames);
        if (new_task == NULL)
            return 4;

        kfree(file);

        current_task->state = TASK_BLOCKED;
        r->rax = 0;
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
        uint32_t max_len = (uint32_t)r->rsi;

        char *tmpbuf = kmalloc(max_len);
        if (!tmpbuf)
        {
            r->rax = 1;
            return (uint64_t)r;
        }
        input(tmpbuf, max_len);
        copy_to_user(buf, tmpbuf, strlen(tmpbuf) + 1);

        r->rax = 0;
        return (uint64_t)r;
    }
    case 5: // cls
    {
        cls();
        r->rax = 0;
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
        /*int type = (int)r->rdi;
        char *ret_buf = (char *)r->rsi;

        if (type == 0)
            copy_to_user(ret_buf, UNAME[0], strlen(UNAME[0]));
        else if (type == 1)
            copy_to_user(ret_buf, UNAME[1], strlen(UNAME[1]));
        else if (type == 2)
            copy_to_user(ret_buf, UNAME[2], strlen(UNAME[2]));
        r->rax = 1;
        return (uint64_t)r;*/
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
        extern uint8_t *task_heap;
        size_t increment = (size_t)r->rdi;
        uint8_t *previous_heap_end = task_heap;
        task_heap += increment;
        for (int i = 0; i < (increment / PAGE_SIZE) + 1; i++)
        {
            task_heap += i;
            void *frame = pmm_alloc_frame();
            vmm_map_page(kernel_pml4_virt, (uintptr_t)task_heap, (uintptr_t)frame, VMM_PRESENT | VMM_WRITE | VMM_USER);
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
        int x = vfs_open(path, flags, &size, current_task->fd_table);
        return x;
    }
    case 11: // file read
    {
        int fd = (int)r->rdi;
        uint64_t size = (uint64_t)r->rsi;
        void *ptr = (void *)r->rdx;

        void *tmpbuf = kmalloc(size);

        int bytes_read = vfs_read(fd, tmpbuf, size);
        copy_to_user(ptr, tmpbuf, bytes_read);
        kfree(tmpbuf);

        return bytes_read;
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
        return vfs_close(fd, current_task->fd_table);
    }
    case 14: // fork
    {
        extern task_t *current_task;

        if (!current_task)
            return 1;

        task_t *new = (task_t *)kmalloc(sizeof(task_t));
        if (!new)
            return 1;

        memcpy((uint8_t *)new, (uint8_t *)current_task, sizeof(*current_task));
        if (memcmp(new, current_task, sizeof(*current_task)) != 0)
            return 2;

        new->state = TASK_NEW;
        new->parent = current_task;
        new->pid = current_task->pid + 1;

        return task_insert(new);
    }
    case 15: // soon to be execve
    {
        char *path = (char *)r->rdi;
        char **argv_ptr = (char **)r->rsi;
        int argc = (int)r->rdx;

        if (argc < 1 || argc > 32)
        {
            log(LOG_ERR, "Error: argc < 1 OR argc > 32, aborting new process");
            schedule(r);
            return (uint64_t)r;
        }

        char *argv[argc];
        for (int i = 0; i < argc; i++)
        {
            size_t len = strlen(argv_ptr[i]);
            argv[i] = (char *)kmalloc(len + 1);
            memcpy(argv[i], argv_ptr[i], len + 1);
        }

        return execve(path, argv, argc, NULL);
    }
    case 16: // get current task's PID
    {
        // r->rax = current_task->pid;
        // return (uint64_t)r;
        return (uint64_t)current_task->pid;
    }
    default:
    {
        r->rax = ENOSYS;
        return (uint64_t)r;
    }
    }
}
