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
#include <syscall.h>
#include <string.h>

int main(int argc, char **argv)
{
    char kname[32];
    char krelease[16];
    char kbuild[8];

    sysinfo(0, kname);
    sysinfo(1, krelease);
    sysinfo(2, kbuild);

    if (argc > 1)
    {
        if (argv[1][1] == 's')
            printf("%s ", kname);
        if (argv[1][1] == 'r')
            printf("%s-b%s", krelease, kbuild);
        if (argv[1][1] == 'o')
            printf("%s ", OSVER);
    }
    else
        printf("%s", krelease);

    printf("\n");

    return 0;
}
