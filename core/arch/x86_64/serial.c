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

#include <drivers/serial.h>
#include <drivers/io.h>

int is_transmit_empty()
{
    return inb(COM1 + 5) & 0x20;
}

void putc_serial(char a)
{
    while (is_transmit_empty() == 0)
        ;
    outb(COM1, a);
}

int serial_received()
{
    return inb(COM1 + 5) & 1;
}

char read_serial()
{
    while (serial_received() == 0)
        ;
    return inb(COM1);
}

int init_serial()
{
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);

    outb(COM1 + 4, 0x1E);
    outb(COM1 + 0, 0xAE);

    if (inb(COM1 + 0) != 0xAE)
        return 1;

    outb(COM1 + 4, 0x0F);
    return 0;
}
