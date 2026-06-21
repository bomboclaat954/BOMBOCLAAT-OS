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

#include <stdint.h>
#include <drivers/io.h>
#include <int/int.h>

#define PIT_CHANNEL0 0x40
#define PIT_CMD 0x43
#define PIT_FREQUENCY 1193182
#define PIT_HZ 1000

static volatile unsigned long long ticks = 0;

void pit_init(void)
{
    uint16_t divisor = PIT_FREQUENCY / PIT_HZ;

    outb(PIT_CMD, 0x36);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8));

    /*uint8_t mask = inb(0x21);
    if (mask & 0x01)
        outb(0x21, mask & ~0x01);*/
    outb(0x21, inb(0x21) & ~0x01);
}

void pit_tick(void)
{
    ticks++;
}

uint32_t pit_get_ticks(void)
{
    return ticks;
}

void delay_ms(uint32_t ms)
{
    uint32_t target = ticks + ms;
    while (ticks < target)
        ;
}
