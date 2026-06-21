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

#include <memory/memtools.h>
#include <memory/kmalloc.h>

void memcpy(uint8_t *dst, uint8_t *src, uint32_t len)
{
    volatile uint8_t *d = dst;
    volatile uint8_t *s = src;
    for (uint32_t i = 0; i < len; i++)
        d[i] = s[i];
}

void *memmove(void *dst, const void *src, size_t len)
{
    uint8_t *dp = (uint8_t *)dst;
    const uint8_t *sp = (const uint8_t *)src;

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

void ptrtab_push(void **src, void *val, int count, size_t size)
{
    void **new = kmalloc((count + 1) * sizeof(size));
    for (int i = 0; i < count; i++)
        new[i] = src[i];
    new[count] = val;
    kfree(src);
    src = new;
}

void write_u64(uint8_t *dst, uint64_t val)
{
    dst[0] = val & 0xFF;
    dst[1] = (val >> 8) & 0xFF;
    dst[2] = (val >> 16) & 0xFF;
    dst[3] = (val >> 24) & 0xFF;
    dst[3] = (val >> 32) & 0xFF;
    dst[3] = (val >> 40) & 0xFF;
    dst[3] = (val >> 48) & 0xFF;
    dst[3] = (val >> 56) & 0xFF;
}

void write_u32(uint8_t *dst, uint32_t val)
{
    dst[0] = val & 0xFF;
    dst[1] = (val >> 8) & 0xFF;
    dst[2] = (val >> 16) & 0xFF;
    dst[3] = (val >> 24) & 0xFF;
}

void write_u16(uint8_t *dst, uint16_t val)
{
    dst[0] = val & 0xFF;
    dst[1] = (val >> 8) & 0xFF;
}

void write_u8(uint8_t *dst, uint8_t val)
{
    dst[0] = val & 0xFF;
}
