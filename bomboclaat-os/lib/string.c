/*
 * BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <string.h>

int strlen(char *str)
{
    int len = 0;
    while (str[len] != '\0')
        len++;
    return len;
}

int strcmp(char *s1, char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(char *s1, char *s2, int n)
{
    while (n > 0 && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
        n--;
    }
    if (n == 0)
        return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void strcpy(char *s, char *p)
{
    char *temp1 = s;
    char *temp2 = p;
    while (*temp1 != '\0')
    {
        *temp2 = *temp1;
        temp1++;
        temp2++;
    }
    *temp2 = '\0';
}

char *strstr(char *str, char *substring)
{
    char *a;
    char *b;

    b = substring;

    if (*b == 0)
        return (char *)str;

    for (; *str != 0; str += 1)
    {
        if (*str != *b)
            continue;

        a = str;
        while (1)
        {
            if (*b == 0)
                return (char *)str;

            if (*a++ != *b++)
                break;
        }
        b = substring;
    }

    return NULL;
}

/*char *strtok(char *s, char *delm)
{
    static int currIndex = 0;
    if (!s || !delm || s[currIndex] == '\0')
        return NULL;
    char *W = (char *)kmalloc(sizeof(char) * 100);
    int i = currIndex, k = 0, j = 0;

    while (s[i] != '\0')
    {
        j = 0;
        while (delm[j] != '\0')
        {
            if (s[i] != delm[j])
                W[k] = s[i];
            else
                goto It;
            j++;
        }

        i++;
        k++;
    }
It:
    W[i] = 0;
    currIndex = i + 1;
    return W;
}*/

void strrem(char *str, char *substr)
{
    char *pos = strstr(str, substr);
    int sublen = strlen(substr);
    for (int i = pos - str;; i++)
    {
        str[i] = str[i + sublen];
        if (str[i] == '\0')
            break;
    }
}

void lower(char *str)
{
    for (char *p = str; *p; ++p)
        *p = *p > 0x40 && *p < 0x5B ? *p | 0x60 : *p;
}

void upper(char *str)
{
    for (char *p = str; *p; ++p)
        *p = *p > 0x60 && *p < 0x7B ? *p & ~0x20 : *p;
}

void *clear_str(char *str)
{
    int len = strlen(str);
    for (int i = 0; i < len; i++)
        str[i] = ' ';
    str[len - 1] = '\0';
}

void memcpy(uint8_t *dst, char *src, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        dst[i] = src[i];
}

void *memmove(void *dst, void *src, size_t len)
{
    uint8_t *dp = (uint8_t *)dst;
    uint8_t *sp = (uint8_t *)src;

    if (sp < dp && sp + len > dp)
    {
        sp += len;
        dp += len;
        while (len-- > 0)
            *--dp = *--sp;
    }
    else
    {
        while (len-- > 0)
            *dp++ = *sp++;
    }

    return dst;
}

void *memset(void *ptr, int value, uint32_t num)
{
    unsigned char *p = (unsigned char *)ptr;
    while (num--)
    {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

int memcmp(void *buf1, void *buf2, size_t count)
{
    unsigned char *s1 = (unsigned char *)buf1;
    unsigned char *s2 = (unsigned char *)buf2;

    for (size_t i = 0; i < count; i++)
    {
        if (s1[i] < s2[i])
            return -1;
        else if (s1[i] > s2[i])
            return 1;
    }
    return 0;
}

char *join(char *str1, char *str2, char *output_str, int n)
{
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    for (int i = 0; i < len1; i++)
        output_str[i] = str1[i];
    for (int i = 0; i < len2; i++)
        output_str[len1 + i] = str2[i];
    int start = len1 + len2;
    if (n)
        output_str[start++] = '\n';
    output_str[start] = '\0';
    return output_str;
}
