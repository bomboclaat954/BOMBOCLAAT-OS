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

int user_pf(registers_t *regs, uintptr_t addr)
{
    if (regs->error_code & 0x02)
    {
        void *phys = pmm_alloc_frame();
        if (!phys)
            return 1;

        return vmm_map_page(current_task->pml4, addr, (uintptr_t)phys, VMM_PRESENT | VMM_WRITE | VMM_USER);
    }
    else if (regs->error_code & 0x08)
        return 2;

    return 3;
}

int pf_handler(registers_t *regs, uint64_t addr)
{
    if (!addr)
    {
        log(LOG_ERR, "PID %d: #PF at NULL", current_task->pid);
        signal_send(SIGKILL, current_task->pid);
    }

    if (regs->cs & 0x03)
    {
        if (user_pf(regs, addr) != 0)
            panic("Unfixable #PF in user space", regs, 0);
    }
    else
        panic("#PF in kernel space", regs, 0);

    return 0;
}
