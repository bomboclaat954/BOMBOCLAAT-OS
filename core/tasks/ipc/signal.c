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
#include <memory/userspace.h>
#include <bomboclaat/kprintf.h>

int signal_send(sig_t sig, pid_t target)
{
    if (target == 0)
        return 1; // don't talk to PID 0!

    task_t *target_task = find_by_pid(target);
    if (!target_task || target_task->state == TASK_ZOMBIE)
        return 2;

    if (sig != SIGKILL && sig != SIGTERM)
        return -1;

    uint64_t flags = irq_save();
    target_task->pending_signal = sig;
    int ret = execute_signal(target_task);
    irq_restore(flags);
    return ret;
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
        log(LOG_INFO, "PID %d: received SIGKILL", target->pid);
        task_terminate(target, 128 + SIGKILL);
        if (target == current_task)
            sched();
        return 0;
    }
    case SIGTERM:
    {
        // A guy opens his door, calls his family to say goodbye and gets shot
        log(LOG_INFO, "PID %d: received SIGTERM", target->pid);

        if (!target->sigterm_handler_rip)
        {
            task_terminate(target, 128 + SIGTERM);
            if (target == current_task)
                sched();
            return 0;
        }

        if (target->state == TASK_BLOCKED)
            target->state = TASK_READY;
        return 0;
    }
    default:
    {
        target->pending_signal = 0;
        return -1;
    }
    }
}

void signal_check_user(uint64_t *user_rip, uint64_t *user_rsp)
{
    task_t *t = current_task;
    if (!t->pending_signal)
        return;

    int sig = t->pending_signal;
    t->pending_signal = 0;

    if (sig == SIGTERM && t->sigterm_handler_rip)
    {
        uint64_t sp = ((*user_rsp - 128) & ~0xFULL) - 8;
        uint64_t ret_addr = USER_SIGTRAMP_VIRT;

        if (copy_to_user((void *)sp, &ret_addr, sizeof(ret_addr)) == 0)
        {
            *user_rsp = sp;
            *user_rip = t->sigterm_handler_rip;
            t->sigterm_handler_rip = 0;
            return;
        }
    }

    task_terminate(t, 128 + sig);
    sched();
}
