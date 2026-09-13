/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SIGNAL_H
#define SIGNAL_H
#include <tasks/tasks.h>
#include <bomboclaat/types.h>

#define SIGKILL 1
#define SIGTERM 2

int signal_send(sig_t sig, pid_t target);
int execute_signal(task_t *target);

#endif
