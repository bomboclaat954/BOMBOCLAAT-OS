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
#include <bomboclaat/kprintf.h>
#include <x86_64/gdt_tss.h>

extern void reap_zombie(void);
extern task_t *tasks[MAX_TASKS];
extern void set_syscall_kernel_stack(uint64_t rsp);

task_t *pick_next_task()
{
    if (current_task->state == TASK_RUNNING)
        current_task->state = TASK_READY;

    task_t *next = current_task->next;
    int safety_counter = 0;

    while (next->state != TASK_READY && (safety_counter < 64))
    {
        next = next->next;
        safety_counter++;
    }

    if (next->state != TASK_READY)
    {
        task_t *search = current_task;
        do
        {
            if (search->pid == 0)
            {
                next = search;
                break;
            }
            search = search->next;
        } while (search != current_task);
    }

    current_task = next;
    current_task->state = TASK_RUNNING;

    uint64_t *sentinel = (uint64_t *)(current_task->kstack_frames[3] + hhdm_offset + 8);
    if (*sentinel != TASK_STACK_SENTINEL)
        log(LOG_ERR, "PID %d: stack sentinel is corrupted", current_task->pid);

    return current_task;
}

void sched(void)
{
    asm volatile("cli");
    reap_zombie();

    task_t *prev = current_task;
    task_t *next = pick_next_task();

    if (next == prev)
        return;

    if (!next)
    {
        log(LOG_ERR, "No next task");
        next = find_by_pid(0);
    }

    if (next->kstack_top)
        tss.rsp0 = next->kstack_top;

    set_syscall_kernel_stack(next->kstack_top);
    vmm_switch_pml4(next->pml4);
    cpu_switch_context(&prev->kernel_rsp, next->kernel_rsp);
}
