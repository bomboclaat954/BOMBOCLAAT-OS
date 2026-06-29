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
#include <bomboclaat/globals.h>
#include <bomboclaat/kprintf.h>
#include <drivers/screen.h>
#include <drivers/io.h>
#include <drivers/acpi.h>

void reg_dump(registers_t *r)
{
    asm volatile(
        "mov %%rax, %0\n"
        "mov %%rbx, %1\n"
        "mov %%rcx, %2\n"
        "mov %%rdx, %3\n"
        "mov %%rsp, %4\n"
        "mov %%rbp, %5\n"
        "mov %%rsi, %6\n"
        "mov %%rdi, %7\n"
        : "=m"(r->rax), "=m"(r->rbx), "=m"(r->rcx), "=m"(r->rdx),
          "=m"(r->rsp), "=m"(r->rbp), "=m"(r->rsi), "=m"(r->rdi));

    asm volatile(
        "call 1f\n"
        "1: pop %0\n"
        : "=r"(r->rip));

    asm volatile(
        "mov %%cs, %0\n"
        "mov %%r8, %1\n"
        "mov %%r9, %2\n"
        "mov %%r10, %3\n"
        "mov %%r11, %4\n"
        "mov %%r12, %5\n"
        "mov %%r13, %6\n"
        "mov %%r14, %7\n"
        "mov %%r15, %8\n"
        : "=m"(r->cs), "=m"(r->r8), "=m"(r->r9), "=m"(r->r10),
          "=m"(r->r11), "=m"(r->r12), "=m"(r->r13), "=m"(r->r14),
          "=m"(r->r15));
}

void panic(char *msg, registers_t *r, int from_cpu)
{
    color(0xFF0000, 0);
    kprintf("       *** KERNEL PANIC ***\n");
    color(0xFFFFFF, 0);

    if (!r)
    {
        registers_t regs;
        reg_dump(&regs);
        r = &regs;
    }

    uint64_t cr0, cr2, cr3, cr4;
    asm volatile(
        "mov %%cr0, %0\n"
        "mov %%cr2, %1\n"
        "mov %%cr3, %2\n"
        "mov %%cr4, %3\n"
        : "=r"(cr0), "=r"(cr2), "=r"(cr3), "=r"(cr4));

    kprintf("RAX: %x        R9 : %x\n", r->rax, r->r9);
    kprintf("RBX: %x        R10: %x\n", r->rbx, r->r10);
    kprintf("RCX: %x        R11: %x\n", r->rcx, r->r11);
    kprintf("RDX: %x        R12: %x\n", r->rdx, r->r12);
    kprintf("RSI: %x        R13: %x\n", r->rsi, r->r13);
    kprintf("RDI: %x        R14: %x\n", r->rdi, r->r14);
    kprintf("RBP: %x        R15: %x\n", r->rbp, r->r15);
    kprintf("RSP: %x        CR0: %x\n", r->rsp, cr0);
    kprintf("RIP: %x        CR2: %x\n", r->rip, cr2);
    kprintf("CS : %x        CR3: %x\n", r->cs, cr3);
    kprintf("R8 : %x        CR4: %x\n", r->r8, cr4);
    kprintf("Message: %s\n", msg);
    kprintf("Press ESC to shut down or ENTER to reboot\n");
    while (1)
    {
        if (inb(0x64) & 1)
        {
            int scancode = inb(0x60);
            if (scancode == 0x01)
                acpi_shutdown();
            else if (scancode == 0x1C)
                acpi_reboot();
        }
    }
}
