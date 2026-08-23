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
#include <sys/utsname.h>
#include <malloc.h>

int main(int argc, char **argv)
{
    struct utsname name;

    if (uname(&name) != 0)
        return 1;

    printf("%s %s %s %s %s\n", name.sysname, name.release, name.version, name.nodename, name.machine);

    //! WARNING: THIS SHIT BELOW CAUSES SOME STUPID ERRORS, DON'T USE IT
    /*if (argc < 2)
    {
        printf("%s\n", name.sysname);
        return 0;
    }
    else
    {
        if (contains(argv[1], 'r'))
            printf("%s\n", name.release);
        else if (contains(argv[1], 's'))
            printf("%s\n", name.sysname);
        else if (contains(argv[1], 'm'))
            printf("%s\n", name.machine);
        else if (contains(argv[1], 'n'))
            printf("%s\n", name.nodename);
        else if (contains(argv[1], 'v'))
            printf("%s\n", name.version);
        else if (contains(argv[1], 'a'))
            printf("%s %s %s %s %s\n", name.sysname, name.release, name.version, name.nodename, name.machine);
    }*/

    return 0;
}
