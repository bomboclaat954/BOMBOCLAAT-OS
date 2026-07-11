/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STRING_H
#define STRING_H
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int strlen(const char *str);
    int strcmp(const char *s1, const char *s2);
    int strncmp(const char *s1, const char *s2, int n);
    void strcpy(char *s, char *p);
    void strncpy(char *dst, const char *src, uint64_t n);
    char *strstr(char *str, char *substring);
    char *strtok(char *s, char *delm);
    void strrem(char *str, char *substr);
    int atoi(char *str);
    int contains(char *str, char c);
    char *dtoa(double num, char *str, int precision);
    char *itoa(long num, char *str, int base);
    void reverse(char *str, int length);
    void to_lower_case(char *str);
    void to_upper_case(char *str);
    char *join(char *str1, char *str2, int n);
    int is_number(char *x);
    void *clear_str(char *str);
    int input_key();
    void *input(char *buf);
    void *input_passwd(char *buf, int len);
    char *strchr(const char *s, int c);
    int index(char *str, char x);
    void delete_char(char *str, int index);

#ifdef __cplusplus
}
#endif

#endif
