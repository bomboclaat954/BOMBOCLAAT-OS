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

void set_syscall_kernel_stack(uint64_t rsp)
{
    cpu_data.kernel_rsp = rsp;
}

uint64_t get_user_rsp(void)
{
    return cpu_data.user_rsp;
}

uint64_t syscall_handler(syscall_ctx_t *ctx)
{
    extern task_t *current_task;

#ifdef __ARCH_X86_64
    current_task->cpu_ctx.rax = ctx->sys_num;
    current_task->cpu_ctx.rbx = ctx->rbx;
    current_task->cpu_ctx.rcx = ctx->rip;
    current_task->cpu_ctx.rdx = ctx->arg3;
    current_task->cpu_ctx.rbp = ctx->rbp;
    current_task->cpu_ctx.rdi = ctx->arg1;
    current_task->cpu_ctx.rsi = ctx->arg2;
    current_task->cpu_ctx.r8 = ctx->arg5;
    current_task->cpu_ctx.r9 = ctx->arg6;
    current_task->cpu_ctx.r10 = ctx->arg4;
    current_task->cpu_ctx.r11 = ctx->rflags;
    current_task->cpu_ctx.r12 = ctx->r12;
    current_task->cpu_ctx.r13 = ctx->r13;
    current_task->cpu_ctx.r14 = ctx->r14;
    current_task->cpu_ctx.r15 = ctx->r15;
    current_task->cpu_ctx.rip = ctx->rip;
    current_task->cpu_ctx.rflags = ctx->rflags;
    current_task->cpu_ctx.cs = 0x43;
    current_task->cpu_ctx.ss = 0x3B;
    current_task->cpu_ctx.rsp = get_user_rsp();
#endif

    switch (ctx->sys_num)
    {
    case 1: // getpid
    {
        return current_task->pid;
    }
    case 2: // fork
    {
        pid_t res = fork(ctx);
        return res;
    }
    case 3: // execve
    {
        char *path = (char *)ctx->arg1;
        char **argv = (char **)ctx->arg2;

        current_task->state = TASK_BLOCKED;

        struct execve_args e_args = {
            .path = path,
            .argv = argv,
            .envp = NULL,
        };
        return execve(e_args);
    }
    case 4: // waitpid
    {
        int pid = (int)ctx->arg1;
        int status = waitpid(pid);
        return status;
    }
    case 5: // exit (current task)
    {
        int stat = (int)ctx->arg1;
        exit(stat);
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
        struct utsname *buf = (struct utsname *)ctx->arg1;
        extern struct utsname utsname;

        memset(buf->sysname, 0, _UTSNAME_LENGTH);
        memset(buf->nodename, 0, _UTSNAME_LENGTH);
        memset(buf->release, 0, _UTSNAME_LENGTH);
        memset(buf->version, 0, _UTSNAME_LENGTH);
        memset(buf->machine, 0, _UTSNAME_LENGTH);

        if (copy_to_user(buf->sysname, utsname.sysname, _UTSNAME_LENGTH) != 0)
            return -1;
        if (copy_to_user(buf->nodename, utsname.nodename, _UTSNAME_LENGTH) != 0)
            return -1;
        if (copy_to_user(buf->release, utsname.release, _UTSNAME_LENGTH) != 0)
            return -1;
        if (copy_to_user(buf->version, utsname.version, _UTSNAME_LENGTH) != 0)
            return -1;
        if (copy_to_user(buf->machine, utsname.machine, _UTSNAME_LENGTH) != 0)
            return -1;

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
    case 9: // brk
    {
        // TODO: fix it (but execve has to be fixed first)
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

        int bytes_read = vfs_read(fd, current_task->fd_table, tmpbuf, size);
        copy_to_user(ptr, tmpbuf, bytes_read);
        kfree(tmpbuf);

        return bytes_read;
    }
    case 12: // file write
    {
        int fd = (int)ctx->arg1;
        uint64_t size = (uint64_t)ctx->arg2;
        void *buf = (void *)ctx->arg3;
        return vfs_write(fd, current_task->fd_table, buf, size);
    }
    case 13: // file close
    {
        int fd = (int)ctx->arg1;
        return vfs_close(fd, current_task->fd_table);
    }
    case 14: // cls, TO BE REMOVED ONCE USER-SIDE SCREEN DRIVER IS DONE
    {
        cls();
        return 0;
    }
    case 15: // spawn (skip fork, just execute)
    {
        char *path = (char *)ctx->arg1;
        char **argv = (char **)ctx->arg2;

        struct execve_args e_args = {
            .path = path,
            .argv = argv,
            .envp = NULL,
        };

        return spawn(e_args);
    }
    default:
        return -1;
    }
}
