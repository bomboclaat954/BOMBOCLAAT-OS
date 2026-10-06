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

#include <bomboclaat/types.h>
#include <tasks/tasks.h>
#include <tasks/fork.h>
#include <memory/kmalloc.h>
#include <memory/memtools.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <bomboclaat/syscall.h>
#include <lib/string.h>
#include <errno.h>

int fork(syscall_ctx_t *ctx)
{
    task_t *parent = current_task;
    if (!parent || parent == kernel_task)
        return -EPERM;

    uint64_t flags = irq_save();
    int result;

    if (find_free_slot() < 0)
    {
        result = -EAGAIN;
        goto out;
    }

    task_t *child = task_alloc();
    if (!child)
    {
        result = -ENOMEM;
        goto out;
    }

    child->pml4 = vmm_clone_user_space(parent->pml4);
    if (!child->pml4)
    {
        task_release(child);
        result = -ENOMEM;
        goto out;
    }

    child->mm = parent->mm;
    child->sigterm_handler_rip = parent->sigterm_handler_rip;
    task_set_name(child, parent->name);
    task_inherit_fds(child, parent);

    task_fpu_save(parent);
    memcpy((uint8_t *)task_fpu_area(child), (uint8_t *)task_fpu_area(parent), 512);

    memset(&child->cpu_ctx, 0, sizeof(context_t));
    child->cpu_ctx.r15 = ctx->r15;
    child->cpu_ctx.r14 = ctx->r14;
    child->cpu_ctx.r13 = ctx->r13;
    child->cpu_ctx.r12 = ctx->r12;
    child->cpu_ctx.r11 = ctx->rflags;
    child->cpu_ctx.r10 = ctx->arg4;
    child->cpu_ctx.r9 = ctx->arg6;
    child->cpu_ctx.r8 = ctx->arg5;
    child->cpu_ctx.rbp = ctx->rbp;
    child->cpu_ctx.rdi = ctx->arg1;
    child->cpu_ctx.rsi = ctx->arg2;
    child->cpu_ctx.rdx = ctx->arg3;
    child->cpu_ctx.rcx = ctx->rip;
    child->cpu_ctx.rbx = ctx->rbx;
    child->cpu_ctx.rax = 0;
    child->cpu_ctx.rip = ctx->rip;
    child->cpu_ctx.cs = USER_CS;
    child->cpu_ctx.ss = USER_SS;
    child->cpu_ctx.rsp = ctx->user_rsp;
    child->cpu_ctx.rflags = (ctx->rflags & USER_RFLAGS_MASK) | 0x202;

    child->pid = new_pid();
    build_kernel_frame(child);

    task_link(parent, child);
    child->state = TASK_READY;
    result = child->pid;

out:
    irq_restore(flags);
    return result;
}
