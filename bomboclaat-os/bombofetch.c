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

#include <bomboclaat.h>

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

int main()
{
    char kname[32];
    char krelease[16];
    char cpu[48];
    uintptr_t memfree = 0;
    uintptr_t memtotal = 0;

    sysinfo(0, kname);
    sysinfo(1, krelease);
    sysinfo(2, cpu);

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
        /*case 2:
            printf("%s OS: %s\n", logo[i], OSVER);
            break;
        case 3:
            printf("%s  Kernel: %s v%s\n", logo[i], kname, krelease);
            break;*/
        default:
            printf("%s\n", logo[i]);
            break;
        }
    }

    return 0;
}
