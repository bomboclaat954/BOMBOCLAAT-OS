/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef int pid_t;

pid_t sys_fork();
int sys_execve(char *path, char **argv);
int sys_waitpid(pid_t pid);
int sys_exit(int code);
int sys_spawn(char *path, char **argv);
