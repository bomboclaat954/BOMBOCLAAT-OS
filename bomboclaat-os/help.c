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

int main(int argc, char **argv)
{
    printf("Available commands:\n");
    printf("    bombofetch      - show hardware and software info\n");
    printf("    power [OPT]     - shut down or reboot\n");
    printf("    uname [opt]     - show kernel name and version\n");
    printf("    clear           - clear screen\n");

    printf("[OPT]   - obligatory\n");
    printf("[opt]   - optional\n");
    return 0;
}
