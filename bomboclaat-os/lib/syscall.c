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

#include <syscall.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/utsname.h>
typedef int pid_t;

int uname(struct utsname *_utsname)
{
    int res = 0;
    asm volatile(
        "syscall"
        : "=a"(res)
        : "a"(7), "D"(_utsname)
        : "memory");
    return res;
}

pid_t sys_fork()
{
    pid_t res = 0;
    asm volatile(
        "syscall"
        : "=a"(res)
        : "a"(2)
        : "rcx", "r11", "memory");
    return res;
}

int sys_execve(char *path, char **argv)
{
    int res = 0;

    asm volatile(
        "syscall"
        : "=a"(res)
        : "a"(3), "D"(path), "S"(argv)
        : "rcx", "r11", "memory");

    return res;
}

int sys_waitpid(pid_t pid)
{
    int res = 0;
    asm volatile(
        "syscall"
        : "=a"(res)
        : "a"(4), "D"(pid)
        : "rcx", "r11", "memory");
    return res;
}

int sys_exit(int code)
{
    int res = 0;
    asm volatile(
        "syscall"
        : "=a"(res)
        : "a"(5)
        : "rcx", "r11", "memory");
    return res;
}
