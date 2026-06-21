/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CPPDEF_H
#define CPPDEF_H
#include <stddef.h>

void *operator new(size_t size);
void operator delete(void *p, size_t size);

#endif
