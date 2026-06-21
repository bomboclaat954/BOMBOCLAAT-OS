/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PANIC_H
#define PANIC_H
#include <int/int.h>

void panic(char *msg, registers_t *r, int from_cpu);

#endif
