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

void outb(unsigned short port, unsigned char val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

unsigned char inb(unsigned short port)
{
    unsigned char val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port) : "memory");
    return val;
}

void outw(unsigned short port, unsigned short val)
{
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

uint16_t inw(unsigned short port)
{
    uint16_t val;
    asm volatile("inw %w1, %w0" : "=a"(val) : "Nd"(port) : "memory");
    return val;
}

unsigned char read_cmos(unsigned char reg)
{
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}
