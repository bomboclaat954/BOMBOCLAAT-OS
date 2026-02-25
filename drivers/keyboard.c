#include "../include/keyboard.h"

int shift_pressed = 0;
int caps_lock = 0;

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
