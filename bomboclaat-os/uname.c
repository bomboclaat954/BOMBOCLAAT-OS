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
    int s = 0;
    int r = 0;
    int o = 0;
    int v = 0;

    char *ver = "v1.1";
    char kname[128];
    char krelease[128];
    char kbuild[128];

    sysinfo(0, kname);
    sysinfo(1, krelease);
    sysinfo(2, kbuild);

    if (argc < 2)
    {
        printf("%s\n", kname);
        return 0;
    }

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][1] == 's')
            s = 1;
        else if (argv[i][1] == 'r')
            r = 1;
        else if (argv[i][1] == 'o')
            o = 1;
        else if (argv[i][1] == 'v')
            v = 1;
        else if (argv[i][1] == 'h')
        {
            printf("Usage: uname [opt]\n");
            printf("Without option is the same as -s\n");
            printf("    -s kernel name\n");
            printf("    -r kernel release\n");
            printf("    -o OS version\n");
            printf("    -v uname version\n");
            return 0;
        }
    }
    if (s)
        printf("%s ", kname);
    if (r)
        printf("%s.%s ", krelease, kbuild);
    if (o)
        printf("%s ", OSVER);
    if (v)
        printf("%s ", ver);
    printf("\n");

    return 0;
}
