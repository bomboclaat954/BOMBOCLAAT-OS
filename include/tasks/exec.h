/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef EXEC_H
#define EXEC_H

int execve(char *path, char *argv[], char *envp[]);

#endif
