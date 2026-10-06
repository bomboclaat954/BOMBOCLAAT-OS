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
 * This is a completely new Mierdux syscall interface.
 * Instead of using old INT 0x80 it uses new, faster SYSCALL instruction
 * Some syscalls were removed because they're useless and there's better
 * way to do the same. For example old syscall 1 (printf) was dropped.
 * Instead I'll write an user-side screen driver, which will handle that.
 */

#include <x86_64/cpu.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/globals.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/initramfs.h>
#include <bomboclaat/syscall.h>
#include <bomboclaat/utsname.h>
#include <bomboclaat/types.h>
#include <tasks/loader.h>
#include <tasks/fork.h>
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
#include <tasks/exit.h>
#include <tasks/ipc/signal.h>
#include <fs/vfs.h>
#include <stddef.h>

#define IA32_EFER 0xC0000080
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084
#define KERNEL_STACK_SIZE 0x4000
#define SYSCALL_FMASK (0x200 | 0x100 | 0x400 | 0x40000)
#define SYSCALL_MAX_IO (1024 * 1024)

#define EFER_SCE (1ULL << 0)

static uint8_t syscall_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));

uint64_t syscall_kernel_rsp = 0;

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
    wrmsr(IA32_FMASK, SYSCALL_FMASK);

    syscall_kernel_rsp = (uint64_t)(syscall_stack + KERNEL_STACK_SIZE);
}

void set_syscall_kernel_stack(uint64_t rsp)
{
    syscall_kernel_rsp = rsp;
}

static uint64_t syscall_dispatch(syscall_ctx_t *ctx);

uint64_t syscall_handler(syscall_ctx_t *ctx)
{
    extern task_t *current_task;

    uint64_t ret = syscall_dispatch(ctx);

    if (current_task->pending_signal)
        signal_check_user(&ctx->rip, &ctx->user_rsp);

    return ret;
}

static uint64_t syscall_dispatch(syscall_ctx_t *ctx)
{
    extern task_t *current_task;

    switch (ctx->sys_num)
    {
    case 1: // getpid
    {
        return current_task->pid;
    }
    case 2: // fork
    {
        return (uint64_t)(int64_t)fork(ctx);
    }
    case 3: // execve
    {
        struct execve_args e_args = {
            .path = (char *)ctx->arg1,
            .argv = (char **)ctx->arg2,
            .envp = NULL,
        };
        return (uint64_t)(int64_t)execve(e_args);
    }
    case 4: // waitpid
    {
        return (uint64_t)(int64_t)waitpid((int)ctx->arg1);
    }
    case 5: // exit (current task)
    {
        exit((int)ctx->arg1);
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
        extern struct utsname utsname;
        return copy_to_user((void *)ctx->arg1, &utsname, sizeof(struct utsname)) == 0 ? 0 : (uint64_t)-1;
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
    case 9: // sbrk (returns previous break or -1)
    {
        return task_sbrk(current_task, (int64_t)ctx->arg1);
    }
    case 10: // file open
    {
        char path[256];
        if (copy_string_from_user(path, (const char *)ctx->arg1, sizeof(path)) < 0)
            return -1;
        if (path[0] != '/')
            return -1;

        uint64_t size = 0;
        return (uint64_t)(int64_t)vfs_open(path, (int)ctx->arg2, &size, current_task->fd_table);
    }
    case 11: // file read
    {
        int fd = (int)ctx->arg1;
        uint64_t size = ctx->arg2;
        if (size > SYSCALL_MAX_IO)
            size = SYSCALL_MAX_IO;
        if (size == 0)
            return 0;

        void *tmpbuf = kmalloc(size);
        if (!tmpbuf)
            return -1;

        int bytes_read = vfs_read(fd, current_task->fd_table, tmpbuf, size);
        if (bytes_read > 0 && copy_to_user((void *)ctx->arg3, tmpbuf, (uint32_t)bytes_read) != 0)
            bytes_read = -1;
        kfree(tmpbuf);

        return (uint64_t)(int64_t)bytes_read;
    }
    case 12: // file write
    {
        int fd = (int)ctx->arg1;
        uint64_t size = ctx->arg2;
        if (size > SYSCALL_MAX_IO)
            size = SYSCALL_MAX_IO;
        if (size == 0)
            return 0;

        void *tmpbuf = kmalloc(size);
        if (!tmpbuf)
            return -1;

        int written = -1;
        if (copy_from_user(tmpbuf, (void *)ctx->arg3, (uint32_t)size) == 0)
            written = vfs_write(fd, current_task->fd_table, tmpbuf, size);
        kfree(tmpbuf);

        return (uint64_t)(int64_t)written;
    }
    case 13: // file close
    {
        return (uint64_t)(int64_t)vfs_close((int)ctx->arg1, current_task->fd_table);
    }
    case 14: // cls, TO BE REMOVED ONCE USER-SIDE SCREEN DRIVER IS DONE
    {
        cls();
        return 0;
    }
    case 15: // spawn (skip fork, just execute; returns child's PID)
    {
        struct execve_args e_args = {
            .path = (char *)ctx->arg1,
            .argv = (char **)ctx->arg2,
            .envp = NULL,
        };

        return (uint64_t)(int64_t)spawn(e_args);
    }
    default:
        return -1;
    }
}
