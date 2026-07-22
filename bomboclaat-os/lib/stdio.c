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
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <syscall.h>
#include <drivers/screen.h>

void scanf(char *buf)
{
    // TODO: open /dev/kbd, read scancode from it and convert it to string
    asm volatile(
        "int $0x80"
        :
        : "a"(4), "D"(buf)
        : "memory");
}

void print_dec(unsigned int value, unsigned int width, char *buf, int *ptr)
{
    unsigned int n_width = 1;
    unsigned int i = 9;
    while (value > i && i < UINT32_MAX)
    {
        n_width += 1;
        i *= 10;
        i += 9;
    }

    int printed = 0;
    while (n_width + printed < width)
    {
        buf[*ptr] = '0';
        *ptr += 1;
        printed += 1;
    }

    i = n_width;
    while (i > 0)
    {
        unsigned int n = value / 10;
        int r = value % 10;
        buf[*ptr + i - 1] = r + '0';
        i--;
        value = n;
    }
    *ptr += n_width;
}

void print_hex(unsigned int value, unsigned int width, char *buf, int *ptr)
{
    int i = width;

    if (i == 0)
        i = 8;

    unsigned int n_width = 1;
    unsigned int j = 0x0F;
    while (value > j && j < UINT32_MAX)
    {
        n_width += 1;
        j *= 0x10;
        j += 0x0F;
    }

    while (i > (int)n_width)
    {
        buf[*ptr] = '0';
        *ptr += 1;
        i--;
    }

    i = (int)n_width;
    while (i-- > 0)
    {
        buf[*ptr] = "0123456789ABCDEF"[(value >> (i * 4)) & 0xF];
        *ptr += +1;
    }
}

size_t vasprintf(char *buf, char *fmt, va_list args)
{
    va_list aq;
    va_copy(aq, args);

    char *s;
    int ptr = 0;
    int len = strlen(fmt);
    for (int i = 0; i < len && fmt[i]; ++i)
    {
        if (fmt[i] != '%')
        {
            buf[ptr++] = fmt[i];
            continue;
        }
        ++i;
        unsigned int arg_width = 0;
        while (fmt[i] >= '0' && fmt[i] <= '9')
        {
            arg_width *= 10;
            arg_width += fmt[i] - '0';
            ++i;
        }

        switch (fmt[i])
        {
        case 's':
            s = (char *)va_arg(aq, char *);
            if (!s)
                s = "(null)";
            while (*s)
                buf[ptr++] = *s++;
            break;
        case 'c':
            buf[ptr++] = (char)va_arg(aq, int);
            break;
        case 'x':
            print_hex((unsigned long)va_arg(aq, unsigned long), arg_width, buf, &ptr);
            break;
        case 'd':
            print_dec((int)va_arg(aq, int), arg_width, buf, &ptr);
            break;
        case '%':
            buf[ptr++] = '%';
            break;
        default:
            buf[ptr++] = fmt[i];
            break;
        }
    }

    buf[ptr] = '\0';
    va_end(aq);
    return ptr;
}

int x = 0;
int y = 0;

int printf(char *fmt, ...)
{
    char buf[8192];
    va_list args;

    va_start(args, fmt);
    int out = vasprintf(buf, fmt, args);
    va_end(args);

    asm volatile(
        "int $0x80"
        :
        : "a"(1), "D"(buf)
        : "memory");
    /*int i = 0;
    while (buf[i])
    {
        draw_char(buf[i], x, y, 0xFFFFFF, 0);
        x += 8;
        i++;
    }*/

    return out;
}

int sprintf(char *buf, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int out = vasprintf(buf, fmt, args);
    va_end(args);
    return out;
}

int fopen(char *path, int flags)
{
    int fd;
    asm volatile(
        "int $0x80"
        : "=a"(fd)
        : "a"(10), "D"(path), "S"(flags)
        : "memory");
    return fd;
}

int fread(int fd, void *buf, uint64_t size)
{
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(11), "D"(fd), "S"(size), "d"(buf)
        : "memory");
    return ret;
}

int fwrite(int fd, void *buf, uint64_t size)
{
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(12), "D"(fd), "S"(size), "d"(buf)
        : "memory");
    return ret;
}

int fclose(int fd)
{
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(13), "D"(fd)
        : "memory");
    return ret;
}
