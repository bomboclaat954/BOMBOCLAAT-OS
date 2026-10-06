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

#include <x86_64/cpu.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/kprintf.h>
#include <lib/string.h>
#include <tasks/tasks.h>
#include <tasks/exit.h>
#include <memory/pmm.h>

extern int pf_handler(registers_t *regs, uint64_t addr);

static void user_fault(registers_t *r, const char *what)
{
    log(LOG_ERR, "PID %d (%s): %s at RIP=%x", current_task->pid, current_task->name, what, r->rip);
    task_terminate(current_task, 128 + (int)r->int_no);
    sched();
}

void exception_handler(registers_t *r)
{
    if ((r->cs & 3) && r->int_no != 2 && r->int_no != 8 && r->int_no != 14 && r->int_no != 18)
    {
        user_fault(r, r->int_no == 0 ? "division by zero" : (r->int_no == 13 ? "general protection fault" : "CPU exception"));
        return;
    }

    switch (r->int_no)
    {
    case 0:
        panic("CPU-EXC: division by zero", r, 1);
        break;
    case 1:
        panic("CPU-EXC: debug", r, 1);
        break;
    case 2:
        panic("CPU-EXC: non-maskable interrupt", r, 1);
        break;
    case 3:
        panic("CPU-EXC: breakpoint", r, 1);
        break;
    case 4:
        panic("CPU-EXC: overflow", r, 1);
        break;
    case 5:
        panic("CPU-EXC: bound range exceeded", r, 1);
        break;
    case 6:
        panic("CPU-EXC: invalid opcode", r, 1);
        break;
    case 7:
        panic("CPU-EXC: devide not available", r, 1);
        break;
    case 8:
        panic("CPU-EXC: double fault", r, 1);
        break;
    case 10:
        panic("CPU-EXC: invalic TSS", r, 1);
        break;
    case 11:
        panic("CPU-EXC: segment not present", r, 1);
        break;
    case 12:
        panic("CPU-EXC: stack-segment fault", r, 1);
        break;
    case 13:
        panic("CPU-EXC: general protection fault", r, 1);
        break;
    case 14:
    {
        uint64_t fault_addr = 0;
        asm volatile("mov %%cr2, %0" : "=r"(fault_addr));

        if (pf_handler(r, fault_addr) != 0)
            panic("CPU-EXC: #PF not handled properly", r, 1);
        break;
    }
    case 16:
        panic("CPU-EXC: x87 float", r, 1);
        break;
    case 17:
        panic("CPU-EXC: alignment check", r, 1);
        break;
    case 18:
        panic("CPU-EXC: machine check", r, 1);
        break;
    case 19:
        panic("CPU-EXC: SIMD float", r, 1);
        break;
    case 20:
        panic("CPU-EXC: virtualization", r, 1);
        break;
    case 21:
        panic("CPU-EXC: control protection", r, 1);
        break;
    case 28:
        panic("CPU-EXC: hypervisor injection", r, 1);
        break;
    case 29:
        panic("CPU-EXC: VMM communication", r, 1);
        break;
    case 30:
        panic("CPU-EXC: security", r, 1);
        break;
    default:
        panic("CPU-EXC: unknown", r, 1);
        break;
    }
}
