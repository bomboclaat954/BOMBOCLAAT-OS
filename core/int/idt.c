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
#include <bomboclaat/kprintf.h>

__attribute__((aligned(0x10))) static idt_entry_t idt[256];

idtr_t idtr;
int vectors[IDT_MAX_DESCRIPTORS];

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags)
{
    idt_entry_t *descriptor = &idt[vector];
    uintptr_t addr = (uintptr_t)isr;

    descriptor->isr_low = addr & 0xFFFF;
    descriptor->kernel_cs = 0x28;
    descriptor->ist = 0;
    descriptor->attributes = flags;
    descriptor->isr_mid = (addr >> 16) & 0xFFFF;
    descriptor->isr_high = (addr >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;
}

void idt_init(void)
{
    extern void isr_stub_default(void);
    extern void isr_stub_128(void);

    idtr.base = (uint64_t)(uintptr_t)&idt[0];
    idtr.limit = sizeof(idt_entry_t) * 256 - 1;

    for (int i = 0; i < 256; i++)
        idt_set_descriptor(i, (void *)isr_stub_default, 0x8E);

    for (int i = 0; i < 32; i++)
        idt_set_descriptor(i, isr_stub_table[i], 0x8E);

    idt_set_descriptor(32, isr_stub_table[32], 0x8E);
    idt_set_descriptor(33, isr_stub_table[33], 0x8E);
    idt_set_descriptor(128, (void *)isr_stub_128, 0xEE); // syscall 0x80

    asm volatile("lidt %0" : : "m"(idtr));
}
