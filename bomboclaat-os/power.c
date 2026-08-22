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

#include <stdio.h>
#include <string.h>

void shutdown()
{
    asm volatile(
        "syscall"
        :
        : "a"(8), "D"(1)
        : "memory");
}

void reboot()
{
    asm volatile(
        "syscall"
        :
        : "a"(8), "D"(0)
        : "memory");
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: power [-s: shutdown, -r: reboot]\n");
        return 1; // idk how's that possible but this 1 causes segfault, if it's 0 it doesn't
    }

    if (strcmp(argv[1], "-s") == 0)
        shutdown();
    else if (strcmp(argv[1], "-r") == 0)
        reboot();
    else
        printf("Invalid option\n");

    return 0;
}
