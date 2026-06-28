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

#include <drivers/io.h>
#include <int/int.h>
#include <memory/vmm.h>

uintptr_t lapic_base_global = 0;

uintptr_t lapic_get_base(void)
{
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(IA32_APIC_BASE_MSR));
    return ((uint64_t)high << 32 | low) & 0xFFFFF000;
}

void lapic_write(uintptr_t lapic_base, uint32_t reg, uint32_t value)
{
    volatile uint32_t *address = (volatile uint32_t *)(lapic_base + reg);
    *address = value;
}

uint32_t lapic_read(uintptr_t lapic_base, uint32_t reg)
{
    volatile uint32_t *address = (volatile uint32_t *)(lapic_base + reg);
    return *address;
}

void lapic_init(uintptr_t lapic_base)
{
    lapic_base_global = lapic_base;
    lapic_write(lapic_base, LAPIC_SVR, lapic_read(lapic_base, LAPIC_SVR) | LAPIC_SVR_ENABLE | 0xFF);
}

void lapic_timer_init(uintptr_t lapic_base, uint32_t count)
{
    lapic_write(lapic_base, LAPIC_TDCR, 0x3);
    lapic_write(lapic_base, LAPIC_TIMER, TIMER_PERIODIC | 32);
    lapic_write(lapic_base, LAPIC_TICR, count);
}

void apic_eoi(uintptr_t lapic_base)
{
    lapic_write(lapic_base, LAPIC_EOI, 0);
}

void lapic_timer_calibrate(uintptr_t lapic_base)
{
    lapic_write(lapic_base, LAPIC_TDCR, 0x3);
    lapic_write(lapic_base, LAPIC_TIMER, 0x10000);
    lapic_write(lapic_base, LAPIC_TICR, 0xFFFFFFFF);

    outb(0x43, 0x30);
    uint16_t ticks_10ms = PIT_FREQUENCY / 100;
    outb(0x40, ticks_10ms & 0xFF);
    outb(0x40, ticks_10ms >> 8);

    while (1)
    {
        outb(0x43, 0xE2);
        if (inb(0x40) & (1 << 7))
            break;
    }

    uint32_t elapsed = 0xFFFFFFFF - lapic_read(lapic_base, LAPIC_TCCR);
    uint32_t ticks_per_ms = elapsed / 10;

    lapic_write(lapic_base, LAPIC_TIMER, TIMER_PERIODIC | 32);
    lapic_write(lapic_base, LAPIC_TICR, ticks_per_ms);
}
