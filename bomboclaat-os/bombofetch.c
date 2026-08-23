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

#include <string.h>
#include <stdio.h>
#include <sys/utsname.h>

static char *logo[] = {
    "             . . .                ",
    "              \\|/                ",
    "            `--+--'               ",
    "              /|\\                ",
    "             ' | '                ",
    "               |                  ",
    "               |                  ",
    "           ,--'#`--.              ",
    "           |#######|              ",
    "        _.-'#######`-._           ",
    "     ,-'###############`-.        ",
    "   ,'#####################`,      ",
    "  /#########################\\    ",
    " |###########################|    ",
    "|#############################|   ",
    "|#############################|   ",
    "|#############################|   ",
    "|#############################|   ",
    " |###########################|    ",
    "  \\#########################/    ",
    "   `.#####################,'      ",
    "     `._###############_,'        ",
    "        `--..#####..--'           ",
};

int main(int argc, char **argv)
{
    struct utsname __name;

    if (uname(&__name) != 0)
        return 1;

    for (int i = 0; i < ARRAY_SIZE(logo); i++)
    {
        switch (i)
        {
        case 0:
            printf("%s root@bomboclaat\n", logo[i]);
            break;
        case 1:
            printf("%s  ---------------\n", logo[i]);
            break;
        case 2:
            printf("%s OS: %s\n", logo[i], OSVER);
            break;
        case 3:
            printf("%s  Kernel: %s %s %s\n", logo[i], __name.sysname, __name.release, __name.version);
            break;
        default:
            printf("%s\n", logo[i]);
            break;
        }
    }

    return 0;
}
