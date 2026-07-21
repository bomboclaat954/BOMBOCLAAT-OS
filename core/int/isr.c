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

#include <int/int.h>
#include <bomboclaat/panic.h>
#include <bomboclaat/kprintf.h>
#include <drivers/screen.h>
#include <lib/string.h>
#include <tasks/tasks.h>

void exception_handler(registers_t *r)
{
    switch (r->int_no)
    {
    case 0:
        if (r->error_code & (1 << 2))
        {
            extern task_t *current_task;
            kprintf("Division by zero caused by process PID %d, RIP=%x\n", current_task->pid, r->rip);
            task_exit((context_t *)r);
        }
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
        uint64_t fault_addr;
        asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
        if (r->error_code & (1 << 2))
        {
            extern task_t *current_task;
            kprintf("Segmentation fault caused by process PID %d, at: 0x%x, RIP=%x, err_code=0x%x\n",
                    current_task->pid, fault_addr, r->rip, r->error_code);
            task_exit((context_t *)r);
        }
        char buf[128];
        sprintf(buf, "CPU-EXC: page fault at: %x", fault_addr);
        panic(buf, r, 1);
        break;
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
