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

#include <tasks/exit.h>
#include <tasks/tasks.h>
#include <bomboclaat/types.h>
#include <bomboclaat/kprintf.h>
#include <memory/pmm.h>
#include <errno.h>

int exit(int status)
{
    if (current_task == kernel_task)
        return -EPERM;

    irq_save();
    task_terminate(current_task, status);
    sched();

    while (1)
        asm volatile("hlt");
}

int waitpid(pid_t pid)
{
    uint64_t flags = irq_save();

    task_t *child = find_by_pid(pid);
    if (!child || child->parent != current_task)
    {
        irq_restore(flags);
        return -ECHILD;
    }

    while (child->state != TASK_ZOMBIE)
    {
        if (current_task->pending_signal)
        {
            irq_restore(flags);
            return -EINTR;
        }

        current_task->state = TASK_BLOCKED;
        sched();
    }

    int status = child->exit_code;
    task_reap(child);

    irq_restore(flags);
    return status;
}
