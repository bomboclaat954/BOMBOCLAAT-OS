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

#include <drivers/keyboard.h>
#include <drivers/io.h>
#include <int/int.h>
#include <fs/vfs.h>
#include <fs/tmpfs.h>
#include <memory/stack.h>
#include <memory/memtools.h>
#include <memory/kmalloc.h>
#include <bomboclaat/kprintf.h>
#include <lib/string.h>
#include <stdint.h>

int shift_pressed = 0;
int caps_lock = 0;
stack_t kbd_stack;
int kbd_stack_size = 0;

char get_ascii(unsigned char scancode)
{
    static char map_lower[] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '};
    static char map_upper[] = {
        0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0,
        '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '};

    if (scancode >= 58)
        return 0;

    char c = map_lower[scancode];
    int is_letter = (c >= 'a' && c <= 'z');

    if (is_letter)
    {
        if (shift_pressed ^ caps_lock)
            return map_upper[scancode];
        return map_lower[scancode];
    }
    else
    {
        if (shift_pressed)
            return map_upper[scancode];
        return map_lower[scancode];
    }
}

void keyboard_handler()
{
    uint8_t scancode = inb(0x60);
    if (kbd_stack_size == MAX_SIZE)
    {
        memset(kbd_stack.arr, 0, MAX_SIZE);
        kbd_stack.top = 0;
        kbd_stack_size = 0;
    }
    push(&kbd_stack, scancode);
    kbd_stack_size++;
    return;
}

void keyboard_init()
{
    // TODO: link it do DEVFS

    memset(kbd_stack.arr, 0, MAX_SIZE);
    kbd_stack.top = 0;
    kbd_stack_size = 0;
}
