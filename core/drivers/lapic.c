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

volatile uintptr_t lapic_base = 0;

void lapic_write(uint32_t reg, uint32_t value)
{
    volatile uint32_t *address = (volatile uint32_t *)(lapic_base + reg);
    *address = value;
}

uint32_t lapic_read(uint32_t reg)
{
    volatile uint32_t *address = (volatile uint32_t *)(lapic_base + reg);
    return *address;
}

void lapic_init()
{
    lapic_write(LAPIC_SVR, lapic_read(LAPIC_SVR) | LAPIC_SVR_ENABLE | 0xFF);
}

void lapic_timer_init(uint32_t count)
{
    lapic_write(LAPIC_TDCR, 0x3);
    lapic_write(LAPIC_TIMER, TIMER_PERIODIC | 32);
    lapic_write(LAPIC_TICR, count);
}

void apic_eoi()
{
    lapic_write(LAPIC_EOI, 0);
}

void lapic_timer_calibrate()
{
    lapic_write(LAPIC_TDCR, 0x3);
    lapic_write(LAPIC_TIMER, 0x10000);
    lapic_write(LAPIC_TICR, 0xFFFFFFFF);

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

    uint32_t elapsed = 0xFFFFFFFF - lapic_read(LAPIC_TCCR);
    uint32_t ticks_per_ms = elapsed / 10;

    lapic_write(LAPIC_TIMER, TIMER_PERIODIC | 32);
    lapic_write(LAPIC_TICR, ticks_per_ms);
}
