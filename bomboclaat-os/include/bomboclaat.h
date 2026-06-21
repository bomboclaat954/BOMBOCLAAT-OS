/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#define NULL ((void *)0)

void *uname(int type, char *buf);
int sysexec_elf(char *name);
void *sbrk(size_t increment);
void scanf(char *buf);
int strlen(const char *str);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n);
void strcpy(char *s, char *p);
char *strstr(char *str, char *substring);
// char *strtok(char *s, char *delm);
void strrem(char *str, char *substr);
void lower(char *str);
void upper(char *str);
void *clear_str(char *str);
int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
void *malloc(size_t size);
void free(void *ptr);
void memcpy(uint8_t *dst, const char *src, uint32_t len);
void *memmove(void *dst, const void *src, size_t len);
void *memset(void *ptr, int value, uint32_t num);
int memcmp(const void *buf1, const void *buf2, size_t count);
char *join(char *str1, char *str2, char *output_str, int n);
