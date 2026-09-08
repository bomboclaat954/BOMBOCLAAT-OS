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
#include <memory/kmalloc.h>
#include <memory/memtools.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <bomboclaat/syscall.h>
#include <lib/string.h>

extern uintptr_t build_kernel_frame_from_syscall(task_t *task);
extern uint64_t get_user_rsp(void);

task_t *create_child(task_t *parent, syscall_ctx_t *parent_regs)
{
    if (!parent)
        return NULL;

    task_t *new = (task_t *)kmalloc(sizeof(task_t));
    if (!new)
        return NULL;
    memset(new, 0, sizeof(task_t));

    strcpy(parent->name, new->name);
    for (int i = 0; i < MAX_FILES_PER_TASK; i++)
    {
        if (parent->fd_table[i])
            new->fd_table[i] = parent->fd_table[i];
    }
    new->parent = parent;
    new->pid = new_pid();

    new->pml4 = vmm_clone_user_space(parent->pml4);
    if (!new->pml4)
    {
        kfree(new);
        return NULL;
    }

    new->cpu_ctx = parent->cpu_ctx;
    new->cpu_ctx.rax = 0;

    for (int i = 0; i < 4; i++)
    {
        void *frame = pmm_alloc_frame();
        if (!frame)
        {
            kfree(new);
            return NULL;
        }
        new->kstack_frames[i] = (uintptr_t)frame;
    }
    new->kstack_top = new->kstack_frames[3] + hhdm_offset + PAGE_SIZE;
    *(uint64_t *)(new->kstack_frames[3] + hhdm_offset + 8) = TASK_STACK_SENTINEL;
    new->kernel_rsp = build_kernel_frame_from_syscall(new);

    INIT_LIST_HEAD(&new->children);
    list_add_tail(&new->sibling, &parent->children);

    new->state = TASK_NEW;
    return new;
}

int fork(syscall_ctx_t *ctx)
{
    extern task_t *current_task;

    if (!current_task)
        return -1;

    task_t *new = create_child(current_task, ctx);
    if (!new)
        return -1;

    new->state = TASK_READY;

    if (task_insert(new) < 0)
    {
        list_del(&new->sibling);
        kfree(new);
        return -1;
    }

    new->next = current_task->next;
    current_task->next = new;

    return new->pid;
}
