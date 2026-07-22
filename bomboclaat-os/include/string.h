/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include <stdint.h>
#include <stddef.h>

int strlen(char *str);
int strcmp(char *s1, char *s2);
int strncmp(char *s1, char *s2, int n);
void strcpy(char *s, char *p);
char *strstr(char *str, char *substring);
// char *strtok(char *s, char *delm);
void strrem(char *str, char *substr);
void lower(char *str);
void upper(char *str);
void *clear_str(char *str);
void memcpy(uint8_t *dst, char *src, uint32_t len);
void *memmove(void *dst, void *src, size_t len);
void *memset(void *ptr, int value, uint32_t num);
int memcmp(void *buf1, void *buf2, size_t count);
char *join(char *str1, char *str2, char *output_str, int n);
