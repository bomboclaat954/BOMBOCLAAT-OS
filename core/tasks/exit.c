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

extern task_t *tasks[MAX_TASKS];
extern void reap_zombie();

void remove_child(task_t *child)
{
    list_del(&child->sibling);
}

int exit(int status)
{
    current_task->exit_code = status;
    current_task->state = TASK_ZOMBIE;

    if (current_task->parent && current_task->parent->state == TASK_BLOCKED)
        current_task->parent->state = TASK_READY;

    sched();

    return 0;
}

int waitpid(pid_t pid)
{
    task_t *child = find_by_pid(pid);
    if (!child || child->parent != current_task)
        return -1;

    while (child->state != TASK_ZOMBIE)
        sched();

    int status = child->exit_code;

    int slot = find_in_array(child);
    if (slot >= 0)
        tasks[slot] = NULL;
    list_del(&child->sibling);

    extern task_t *task_to_reap;
    task_to_reap = child;
    reap_zombie();

    return status;
}
