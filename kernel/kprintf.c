#include <bomboclaat-os/kprintf.h>
#include <lib/string.h>
#include <drivers/io.h>
#include <drivers/screen.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

static void print_dec(unsigned int value, unsigned int width, char *buf, int *ptr)
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
    int i = 0;
    char *s;
    int ptr = 0;
    int len = strlen(fmt);
    for (; i < len && fmt[i]; ++i)
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
        case 'a':
            int arg = va_arg(args, int);
            set_color(arg >> 4, arg & 0x0F);
        case 's':
            s = (char *)va_arg(args, char *);
            while (*s)
                buf[ptr++] = *s++;
            break;
        case 'c':
            buf[ptr++] = (char)va_arg(args, int);
            break;
        case 'x':
            print_hex((unsigned long)va_arg(args, unsigned long), arg_width, buf, &ptr);
            break;
        case 'd':
            print_dec((unsigned long)va_arg(args, unsigned long), arg_width, buf, &ptr);
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
    char buf[1024] = {-1};
    va_list args;
    va_start(args, fmt);
    int out = vasprintf(buf, fmt, args);
    va_end(args);
    unsigned char *c = (uint8_t *)buf;
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