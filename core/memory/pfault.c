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
// Mierdux page fault (#PF) handler. Heavily inspired its Linux equivalent.
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <x86_64/cpu.h>
#include <tasks/tasks.h>
#include <tasks/exit.h>
#include <tasks/ipc/signal.h>
#include <bomboclaat/kprintf.h>
#include <bomboclaat/panic.h>

static int user_pf(registers_t *regs, uintptr_t addr)
{
    if (regs->error_code & 0x01)
        return 1;

    if (addr >= USER_SPACE_LIMIT)
        return 2;

    mm_t *mm = &current_task->mm;
    if (addr < mm->stack_start && addr >= mm->stack_limit)
    {
        void *phys = pmm_alloc_frame_zeroed();
        if (!phys)
            return 3;

        if (vmm_map_page(current_task->pml4, addr & ~(uintptr_t)(PAGE_SIZE - 1), (uintptr_t)phys, VMM_PRESENT | VMM_WRITE | VMM_USER) != 0)
        {
            pmm_free_frame(phys);
            return 4;
        }
        return 0;
    }

    return 5;
}

int pf_handler(registers_t *regs, uint64_t addr)
{
    if (!(regs->cs & 0x03))
    {
        char msg[128];
        sprintf(msg, "#PF in kernel space at 0x%x", addr);
        panic(msg, regs, 0);
        return 1;
    }

    if (user_pf(regs, addr) != 0)
    {
        log(LOG_ERR, "PID %d (%s): segmentation fault at 0x%x, RIP=0x%x", current_task->pid, current_task->name, addr, regs->rip);
        task_terminate(current_task, 139);
        sched();
    }

    return 0;
}
