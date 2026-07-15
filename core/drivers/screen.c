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
#include <drivers/io.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <lib/string.h>
#include <fonts/terminus-normal.h>
#include <fs/devfs.h>
#include <bomboclaat/kprintf.h>

int current_fgc = 0xFFFFFF;
int current_bgc = 0x000000;
int cursor_x = 0;
int cursor_y = 0;
int ROWS = 0;
int COLUMNS = 0;

struct limine_framebuffer *fb = 0;
uint32_t fb_offset = 0;
dev_t *framebuffer;
struct dev_ops fbf_ops = {
    .read = NULL,
    .write = fbf_write,
};

void color(int fg, int bg)
{
    if (fg > 0xFFFFFF || bg > 0xFFFFFF)
        return;
    current_fgc = fg;
    current_bgc = bg;
}

void init_screen_driver(struct limine_framebuffer *fbuf)
{
    fb = fbuf;
    COLUMNS = fb->width;
    ROWS = fb->height;
}

void register_framebuffer()
{
    framebuffer = (dev_t *)kmalloc(sizeof(dev_t));
    for (int i = 0; i < MAX_SIZE; i++)
        framebuffer->data_stack.arr[i] = 0;
    framebuffer->data_stack.top = 0;
    framebuffer->data_stack_size = 0;
    framebuffer->name = "fbf";
    framebuffer->ops = &fbf_ops;
    devfs_register_device(framebuffer);
}

int64_t fbf_write(struct vfs_inode *inode, void *buffer, uint64_t size, uint64_t offset)
{
    uint64_t packed = (uint64_t)buffer;
    uint32_t color = (uint32_t)(packed >> 32);
    uint32_t xy = (uint32_t)packed;
    uint16_t x = (uint16_t)(xy >> 16);
    uint16_t y = (uint16_t)xy;
    put_pixel(x, y, color);
    return size;
}

void put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    uint32_t real_y = (y + fb_offset) % fb->height;
    volatile uint32_t *pixel = (uint32_t *)(fb->address + real_y * fb->pitch + x * 4);
    *pixel = color;
}

void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    for (uint32_t dy = 0; dy < h; dy++)
        for (uint32_t dx = 0; dx < w; dx++)
            put_pixel(x + dx, y + dy, color);
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

void draw_string(const char *str, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg)
{
    while (*str)
    {
        draw_char(*str++, x, y, fg, bg);
        x += FONT_COLS;
    }
}

void draw_image(int x, int y, int width, int height, const uint32_t *pixels)
{
    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            uint32_t color = pixels[row * width + col];
            put_pixel(x + col, y + row, color);
        }
    }
}

void update_cursor()
{
    draw_char('_', cursor_x + 1, cursor_y, current_fgc, current_bgc);
}

void putc(char c)
{
    if (cursor_x + FONT_COLS > fb->width)
    {
        cursor_x = 0;
        cursor_y += FONT_ROWS;
    }

    if (cursor_y + FONT_ROWS > fb->height)
        scroll();

    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y += FONT_ROWS;
    }
    else if (c == '\b')
    {
        if (cursor_x >= FONT_COLS)
        {
            cursor_x -= FONT_COLS;
            draw_char(' ', cursor_x, cursor_y, current_fgc, current_bgc);
        }
    }
    else
    {
        draw_char(c, cursor_x, cursor_y, current_fgc, current_bgc);
        cursor_x += FONT_COLS;
    }
}

void puts(char *s, int endl)
{
    while (*s)
        putc(*s++);
    for (int i = 0; i < endl; i++)
        putc('\n');
}

void set_cursor(int x, int y)
{
    cursor_x = x;
    cursor_y = y;
}

void cls()
{
    fill_rect(0, 0, fb->width, fb->height, 0x000000);
    cursor_x = 0;
    cursor_y = 0;
    fb_offset = 0;
}

void scroll()
{
    fb_offset += FONT_ROWS;

    uint32_t clear_row = (fb_offset + fb->height - FONT_ROWS) % fb->height;
    volatile uint8_t *fb_mem = fb->address;
    memset((void *)(fb_mem + clear_row * fb->pitch), 0x00, FONT_ROWS * fb->pitch);

    cursor_y -= FONT_ROWS;
}
