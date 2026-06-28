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

int main(int argc, char **argv)
{
    char *uver = "1.0";
    char kname[32];
    char krelease[32];
    char kbuild[32];

    sysinfo(0, kname);
    sysinfo(1, krelease);
    sysinfo(2, kbuild);

    // int s, r, o, v, help;

    if (argc < 2)
        printf("%s\n", kname);
    else
    {
        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "-s") == 0)
                printf("%s ", kname);
            else if (strcmp(argv[i], "-r") == 0)
                printf("%s ", krelease);
            else if (strcmp(argv[i], "-o") == 0)
                printf("%s ", OSVER);
            else if (strcmp(argv[i], "-v") == 0)
                printf("%s ", uver);
            else if (strcmp(argv[i], "--help") == 0)
            {
                printf("Usage: uname [opt]\n");
                printf("Without option is the same as -s\n");
                printf("    -s kernel name\n");
                printf("    -r kernel release\n");
                printf("    -o OS version\n");
                printf("    -v uname version\n");
            }
            else
                printf("Type uname --help to see possible options\n");
        }
        printf("\n");
    }

    return 0;
}
