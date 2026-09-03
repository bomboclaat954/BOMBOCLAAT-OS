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
#include <stdint.h>

int main()
{
    asm volatile(
        "syscall"
        :
        : "a"(14));

    printf("%s\n", OSVER);

    int status = 0;
    int pid = sys_fork();

    char *argv[2] = {"/bin/shell", NULL};

    sys_execve("/bin/shell", argv);
    status = sys_waitpid(pid);
    printf("/bin/shell ended with status %d\n", status);

    return 0;
}
