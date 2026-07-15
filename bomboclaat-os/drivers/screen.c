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

void put_pixel(uint32_t x, uint32_t y, uint32_t color) // slow as hell but works
{
    int fbf = open("/dev/fbf", 0);
    uint32_t xy = ((uint32_t)x << 16) | y;
    uint64_t buffer = ((uint64_t)color << 32) | xy;
    write(fbf, (void *)buffer, 1);
    close(fbf);
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
