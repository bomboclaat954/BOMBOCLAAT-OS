/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LOADER_H
#define LOADER_H
#include <stddef.h>

void load_bin(void *data, int pages, size_t size);
void load_elf(void *data);

#endif
