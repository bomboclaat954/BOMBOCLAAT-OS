/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include <stdint.h>

void draw_char(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
void draw_string(char *str, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
int init_screen_driver();
