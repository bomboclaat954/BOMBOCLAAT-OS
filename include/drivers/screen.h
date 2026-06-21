/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SCREEN_H
#define SCREEN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <boot/limine.h>

    extern int cursor_x;
    extern int cursor_y;

    extern int ROWS;
    extern int COLUMNS;

    void color(int fg, int bg);
    void init_screen_driver(struct limine_framebuffer *fbuf);
    void put_pixel(uint32_t x, uint32_t y, uint32_t color);
    void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    void draw_char(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
    void draw_string(const char *str, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
    void draw_image(int x, int y, int width, int height, const uint32_t *pixels);
    void update_cursor();
    void putc(char c);
    void puts(char *s, int endl);
    void set_cursor(int x, int y);
    void cls();

#ifdef __cplusplus
}
#endif

#endif
