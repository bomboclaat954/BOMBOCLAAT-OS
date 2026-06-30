/*
 * BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026  Jakub Fietko <fietkojakub@proton.me>
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

extern volatile uintptr_t ioapic_base;

#define IOAPIC_REGSEL ((volatile uint32_t *)(ioapic_base + 0x00))
#define IOAPIC_IOWIN ((volatile uint32_t *)(ioapic_base + 0x10))

uint32_t ioapic_read(uint8_t reg_index)
{
    *IOAPIC_REGSEL = reg_index;
    return *IOAPIC_IOWIN;
}

void ioapic_write(uint8_t reg_index, uint32_t data)
{
    *IOAPIC_REGSEL = reg_index;
    *IOAPIC_IOWIN = data;
}

void ioapic_set_irq(uint8_t gsi, uint8_t vector, uint8_t target_cpu_apic_id)
{
    uint8_t reg_low = 0x10 + (gsi * 2);
    uint8_t reg_high = reg_low + 1;
    uint32_t low_bits = vector;
    uint32_t high_bits = ((uint32_t)target_cpu_apic_id) << 24;

    ioapic_write(reg_low, low_bits);
    ioapic_write(reg_high, high_bits);
}
