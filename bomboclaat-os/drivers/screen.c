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

#include <drivers/screen.h>
#include <fonts/font-limine.h>
#include <bomboclaat.h>

#define USER_FB_VIRT 0x7FFF00000000
static volatile uint32_t *framebuffer = (volatile uint32_t *)USER_FB_VIRT;

uint64_t pitch_bytes = 0;
uint64_t h = 0;
uint64_t w = 0;

void put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    uint64_t pitch_in_pixels = pitch_bytes / sizeof(uint32_t);
    uint64_t index = (y * pitch_in_pixels) + x;
    framebuffer[index] = color;
}

void draw_char(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg)
{
    uint8_t *glyph = font_pixels[(uint8_t)c];
    for (int row = 0; row < FONT_ROWS; row++)
    {
        for (int col = 0; col < FONT_COLS; col++)
        {
            uint32_t color = 0;
            if (FONT_BITS_ORDER == 0)
                color = (glyph[row] >> col) & 1 ? fg : bg; // LSB
            else
                color = (glyph[row] >> (FONT_COLS - 1 - col)) & 1 ? fg : bg; // MSB
            put_pixel(x + col, y + row, color);
        }
    }
}

void draw_string(char *str, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg)
{
    while (*str)
    {
        draw_char(*str++, x, y, fg, bg);
        x += FONT_COLS;
    }
}

int init_screen_driver()
{
    asm volatile(
        "int $0x80"
        : "=a"(pitch_bytes)
        : "a"(6), "D"(0)
        : "memory");
    asm volatile(
        "int $0x80"
        : "=a"(h)
        : "a"(6), "D"(1)
        : "memory");
    asm volatile(
        "int $0x80"
        : "=a"(w)
        : "a"(6), "D"(2)
        : "memory");
    if (pitch_bytes && h && w)
        return 1;
    else
        return 0;
}
