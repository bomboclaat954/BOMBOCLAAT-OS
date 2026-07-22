/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include <stdint.h>

#define NULL ((void *)0)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

void scanf(char *buf);
int printf(char *fmt, ...);
int sprintf(char *buf, char *fmt, ...);
int fopen(char *path, int flags);
int fread(int fd, void *buf, uint64_t size);
int fwrite(int fd, void *buf, uint64_t size);
int fclose(int fd);
