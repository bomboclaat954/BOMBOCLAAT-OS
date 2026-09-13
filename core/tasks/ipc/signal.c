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
// IPC = Inter-Process Communication
#include <tasks/ipc/signal.h>
#include <tasks/tasks.h>

int signal_send(sig_t sig, pid_t target)
{
    if (target == 0)
        return 1; // don't talk to PID 0!

    task_t *target_task = find_by_pid(target);
    if (!target_task)
        return 2;

    target_task->pending_signal = sig;
    return execute_signal(target_task);
}

int execute_signal(task_t *target)
{
    if (!target)
        return 1;

    switch (target->pending_signal)
    {
    case SIGKILL:
    {
        // A guy opens his door and gets shot
        target->state = TASK_ZOMBIE;
        sched();
    }
    case SIGTERM:
    {
        // A guy opens his door, calls his family to say goodbye and gets shot
        target->cpu_ctx.rip = target->sigterm_handler_rip;
        sched();
    }
    default:
    {
        target->pending_signal = 0;
        return -1;
    }
    }
}
