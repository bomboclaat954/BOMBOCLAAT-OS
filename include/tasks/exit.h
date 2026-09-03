/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef EXIT_H
#define EXIT_H
#include <bomboclaat/types.h>

int exit(int status);
int waitpid(pid_t pid);

#endif
