/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MEMTOOLS_H
#define MEMTOOLD_H

#include <stdint.h>
#include <stddef.h>

void memcpy(uint8_t *dst, uint8_t *src, uint32_t len);
void *memmove(void *dst, const void *src, size_t len);
void *memset(void *ptr, int value, uint32_t num);
int memcmp(void *buf1, void *buf2, size_t count);
void ptrtab_push(void **src, void *val, int count, size_t size);
void write_u64(uint8_t *dst, uint64_t val);
void write_u32(uint8_t *dst, uint32_t val);
void write_u16(uint8_t *dst, uint16_t val);
void write_u8(uint8_t *dst, uint8_t val);

#endif
