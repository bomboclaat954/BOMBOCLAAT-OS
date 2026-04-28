#ifndef KEYBOARD_H
#define KEYBOARD_H

extern int shift_pressed;
extern int caps_lock;

char get_ascii(unsigned char scancode);

#endif
