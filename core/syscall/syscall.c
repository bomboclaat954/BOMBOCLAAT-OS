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
// ...That's the way you do it! (see core/int/int128.c)
// Yes that's a Dire Straits reference

/*
 * This is a completely new BOMBOCLAAT Kernel syscall interface.
 * Instead of using old INT 0x80 it uses new, faster SYSCALL instruction
 * Some syscalls were removed because they're useless and there's better
 * way to do the same. For example old syscall 1 (printf) was dropped.
 * Instead I'll write an user-side screen driver, which will handle that.
 */

#include <int/int.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/initramfs.h>
#include <bomboclaat/syscall.h>
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

#define IA32_EFER 0xC0000080
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084
#define IA32_KERNEL_GS_BASE 0xC0000102
#define KERNEL_STACK_SIZE 0x4000

#define EFER_SCE (1ULL << 0)

static uint8_t syscall_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));

typedef struct
{
    uint64_t kernel_rsp;
    uint64_t user_rsp;
} cpu_state_t;

static cpu_state_t cpu_data;

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

    cpu_data.kernel_rsp = (uint64_t)(syscall_stack + KERNEL_STACK_SIZE);
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)&cpu_data);
}

uint64_t syscall_handler(syscall_ctx_t *ctx)
{
    extern task_t *current_task;
    extern task_t *task_list_head;
    switch (ctx->sys_num)
    {
    case 1: // getpid
    {
        return current_task->pid;
    }
    case 2: // fork
    {
        extern task_t *current_task;

        if (!current_task)
            return 1;

        task_t *new = (task_t *)kmalloc(sizeof(task_t));
        if (!new)
            return 2;

        memcpy((uint8_t *)new, (uint8_t *)current_task, sizeof(*current_task));
        if (memcmp(new, current_task, sizeof(*current_task)) != 0)
            return 3;

        new->state = TASK_NEW;
        new->parent_pid = current_task->pid;
        new->pid = current_task->pid + 1;

        return task_insert(new);
    }
    case 3: // execve
    {
        char *path = (char *)ctx->arg1;
        char **argv_ptr = (char **)ctx->arg2;
        int argc = (int)ctx->arg3;

        if (argc < 1 || argc > 32)
        {
            log(LOG_ERR, "Error: argc < 1 OR argc > 32, aborting new process");
            // schedule(r);
            return 1;
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
    case 4:
    {
        // TODO: waitpid
        return 0;
    }
    case 5: // exit
    {
        // TODO: exit
        return 0;
    }
    case 6: // get framebuffer info (RDI = 0 - pitch, RDI = 1 - height, RDI = 2 - width)
    {
        int x = (int)ctx->arg1;
        if (x == 0)
        {
            extern uint64_t fbf_pitch;
            return fbf_pitch;
        }
        else if (x == 1)
        {
            extern uint64_t fbf_height;
            return fbf_height;
        }
        else if (x == 2)
        {
            extern uint64_t fbf_width;
            return fbf_width;
        }
        return 0;
    }
    case 7: // uname
    {
        int type = (int)ctx->arg1;
        char *ret_buf = (char *)ctx->arg2;

        if (type == 0)
            copy_to_user(ret_buf, UNAME[0], strlen(UNAME[0]));
        else if (type == 1)
            copy_to_user(ret_buf, UNAME[1], strlen(UNAME[1]));
        else if (type == 2)
            copy_to_user(ret_buf, UNAME[2], strlen(UNAME[2]));
        return 0;
    }
    case 8: // reboot / shutdown
    {
        int x = (int)ctx->arg1;
        if (x == 0)
            acpi_reboot();
        else if (x == 1)
            acpi_shutdown();
        return 0;
    }
    case 9: // malloc
    {
        // TODO: fix it
        extern vmm_table_t *kernel_pml4_virt;
        extern uint8_t *task_heap;
        size_t increment = (size_t)ctx->arg1;
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
        char *path = (char *)ctx->arg1;
        int flags = (int)ctx->arg2;
        if (path[0] != '/')
            return -1;
        uint64_t size = 0;
        int x = vfs_open(path, flags, &size, current_task->fd_table);
        return x;
    }
    case 11: // file read
    {
        int fd = (int)ctx->arg1;
        uint64_t size = (uint64_t)ctx->arg2;
        void *ptr = (void *)ctx->arg3;

        void *tmpbuf = kmalloc(size);

        int bytes_read = vfs_read(fd, tmpbuf, size);
        copy_to_user(ptr, tmpbuf, bytes_read);
        kfree(tmpbuf);

        return bytes_read;
    }
    case 12: // file write
    {
        int fd = (int)ctx->arg1;
        uint64_t size = (uint64_t)ctx->arg2;
        void *buf = (void *)ctx->arg3;
        return vfs_write(fd, buf, size);
    }
    case 13: // file close
    {
        int fd = (int)ctx->arg1;
        return vfs_close(fd, current_task->fd_table);
    }
    default:
        return -1;
    }
}
