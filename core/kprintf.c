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

#include <bomboclaat/kprintf.h>
#include <lib/string.h>
#include <drivers/io.h>
#include <drivers/screen.h>
#include <int/int.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

static void print_dec(unsigned int value, unsigned int width, char pad_char, char *buf, int *ptr)
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
        buf[*ptr] = pad_char;
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

static void print_hex(unsigned int value, unsigned int width, char *buf, int *ptr)
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

size_t vasprintf(char *buf, const char *fmt, va_list args)
{
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

        char pad_char = ' ';
        if (fmt[i] == '0')
        {
            pad_char = '0';
            ++i;
        }

        unsigned int arg_width = 0;
        while (fmt[i] >= '0' && fmt[i] <= '9')
        {
            arg_width *= 10;
            arg_width += fmt[i] - '0';
            ++i;
        }

        int is_long_long = 0;
        if (fmt[i] == 'l')
        {
            ++i;
            if (fmt[i] == 'l')
            {
                is_long_long = 1;
                ++i;
            }
            else
                is_long_long = 1;
        }

        switch (fmt[i])
        {
        case 's':
            s = (char *)va_arg(args, char *);
            while (*s)
                buf[ptr++] = *s++;
            break;
        case 'c':
            buf[ptr++] = (char)va_arg(args, int);
            break;
        case 'x':
            if (is_long_long)
                print_hex((unsigned long long)va_arg(args, unsigned long long), arg_width, buf, &ptr);
            else
                print_hex((unsigned long)va_arg(args, unsigned long), arg_width, buf, &ptr);
            break;
        case 'd':
        case 'u':
            if (is_long_long)
                print_dec((unsigned long long)va_arg(args, unsigned long long), arg_width, pad_char, buf, &ptr);
            else
                print_dec((unsigned long)va_arg(args, unsigned long), arg_width, pad_char, buf, &ptr);
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
    return ptr;
}

int kprintf(const char *fmt, ...)
{
    char buf[8192];
    va_list args;
    va_start(args, fmt);
    int out = vasprintf(buf, fmt, args);
    va_end(args);
    puts(buf, 0);

    return out;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int out = vasprintf(buf, fmt, args);
    va_end(args);
    return out;
}

int log(log_type type, const char *fmt, ...)
{
    if (type == LOG_OK)
        draw_string("[+]", cursor_x, cursor_y, 0x00FF00, 0);
    else if (type == LOG_ERR)
        draw_string("[-]", cursor_x, cursor_y, 0xFF0000, 0);
    else
        draw_string("[*]", cursor_x, cursor_y, 0x00BCEF, 0);

    cursor_x += 24;
    char buf[8192];
    va_list args;
    va_start(args, fmt);
    int out = vasprintf(buf, fmt, args);
    va_end(args);
    puts(buf, 1);

    return out;
}
