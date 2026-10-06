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

#include <tasks/tasks.h>
#include <memory/pmm.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/globals.h>
#include <x86_64/gdt_tss.h>

extern uint64_t hhdm_offset;
extern void set_syscall_kernel_stack(uint64_t rsp);

static void check_stack_sentinel(task_t *t)
{
    if (!t->kstack_frames[0])
        return;

    uint64_t *sentinel = (uint64_t *)(t->kstack_frames[0] + hhdm_offset);
    if (*sentinel != TASK_STACK_SENTINEL)
        panic("kernel stack overflow", 0, 0);
}

static task_t *pick_next_task(void)
{
    if (current_task->state == TASK_RUNNING)
        current_task->state = TASK_READY;

    task_t *node = current_task->next;
    for (int i = 0; i <= MAX_TASKS + 1; i++)
    {
        if (node != kernel_task && node->state == TASK_READY)
            return node;
        node = node->next;
    }

    return kernel_task;
}

void sched(void)
{
    uint64_t flags = irq_save();

    task_t *prev = current_task;
    task_t *next = pick_next_task();

    next->state = TASK_RUNNING;

    if (next != prev)
    {
        check_stack_sentinel(prev);
        check_stack_sentinel(next);

        current_task = next;
        tss.rsp0 = next->kstack_top;
        set_syscall_kernel_stack(next->kstack_top);

        if (next->pml4 != prev->pml4)
            vmm_switch_pml4(next->pml4);

        task_fpu_save(prev);
        task_fpu_load(next);
        cpu_switch_context(&prev->kernel_rsp, next->kernel_rsp);
    }

    irq_restore(flags);
}
