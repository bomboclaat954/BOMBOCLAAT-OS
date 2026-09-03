/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef EXEC_H
#define EXEC_H

#include <tasks/tasks.h>

/*
    I saw this one in FreeBSD's code and I thought it looks nice
    so I stole the idea and here it is :)
*/
struct execve_args
{
    char *path;
    char **argv;
    char **envp;
};

int execve(struct execve_args args);

#endif
