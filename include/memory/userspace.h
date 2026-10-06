/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef MM_USERSPACE_H
#define MM_USERSPACE_H
#include <stdint.h>

int copy_to_user(void *dst, void *src, uint32_t len);
int copy_from_user(void *dst, void *src, uint32_t len);
int copy_string_from_user(char *dst, const char *src, uint32_t max_len);

#endif
