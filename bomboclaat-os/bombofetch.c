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

// Very early phase of development, some things may not work properly or at all.

int main()
{
    char *shinfo = ""; // malloc(sizeof(char) * 128);
    char kname[20];
    char krelease[8];
    char cpu[48];
    int memused = 0; // malloc(sizeof(char) * 128);
    int memfree = 0; // malloc(sizeof(char) * 128);

    uname(0, kname);
    uname(1, krelease);
    uname(2, cpu);

    printf("             . . .                  root@bomboclaat\n");
    printf("              \\|/                   ---------------\n");
    printf("            `--+--'                 OS: %s\n", OSVER);
    printf("              /|\\                   Kernel: %s %s\n", kname, krelease);
    printf("             ' | '                  Shell: %s\n", shinfo);
    printf("               |                    CPU: %s\n", cpu);
    printf("               |                    Memory: %d / %d MB\n", memused, memfree);
    printf("           ,--'#`--.\n");
    printf("           |#######|\n");
    printf("        _.-'#######`-._\n");
    printf("     ,-'###############`-.\n");
    printf("   ,'#####################`,\n");
    printf("  /#########################\\\n");
    printf(" |###########################|\n");
    printf("|#############################|\n");
    printf("|#############################|\n");
    printf("|#############################|\n");
    printf("|#############################|\n");
    printf(" |###########################|\n");
    printf("  \\#########################/\n");
    printf("   `.#####################,'\n");
    printf("     `._###############_,'\n");
    printf("        `--..#####..--'\n");

    return 0;
}
