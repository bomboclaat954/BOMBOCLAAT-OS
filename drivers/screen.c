#include "../include/screen.h"
#include "../include/string.h"
#include "../include/io.h"

/*
COLOR CODES
    0x00 - black
    0x01 - blue
    0x02 - green
    0x03 - cyan
    0x04 - red
    0x05 - magenta
    0x06 - brown
    0x07 - light gray
    0x08 - dark gray
    0x09 - light blue
    0x0A - light green
    0x0B - light cyan
    0x0C - light red
    0x0D - ligt magenta
    0x0E - yellow
    0x0F - white
*/

int current_color = 0x07;
int current_fgc;
int current_bgc;
int cursor_x;
int cursor_y;
char *video_fb = (char *)0xB8000;

void putc(char c)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
    }
    else if (c == '\b')
    {
        if (cursor_x > 0)
        {
            cursor_x--;
        }
        else if (cursor_y > 3)
        {
            cursor_y--;
            cursor_x = COLUMNS - 1;
        }
        int pos = (cursor_y * COLUMNS + cursor_x) * 2;
        video_fb[pos] = ' ';
    }
    else
    {
        int pos = (cursor_y * COLUMNS + cursor_x) * 2;
        video_fb[pos] = c;
        video_fb[pos + 1] = current_color;
        cursor_x++;
        if (cursor_x >= COLUMNS)
        {
            cursor_x = 0;
            cursor_y++;
        }
    }

    if (cursor_y > ROWS)
    {
        scroll();
    }
    update_hardware_cursor();
}

void puts(char *s, int endl)
{
    while (*s)
        putc(*s++);
    for (int i = 0; i < endl; i++)
    {
        putc('\n');
    }
}

void update_hardware_cursor()
{
    unsigned short pos = cursor_y * COLUMNS + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void scroll()
{
    for (int i = 0; i < (ROWS - 1) * COLUMNS * 2; i++)
    {
        video_fb[i] = video_fb[i + COLUMNS * 2];
    }
    for (int i = (ROWS - 1) * COLUMNS * 2; i < ROWS * COLUMNS * 2; i += 2)
    {
        video_fb[i] = ' ';
        video_fb[i + 1] = 0x07;
    }
    cursor_y = ROWS - 1;
}

void box(int x, int y, char *text)
{
    // ╔ = \xC9; ═ = \xCD; ╗ = \xBB; ║ = \xBA; ╚ = \xC8; ╝ = \0xBC
    set_cursor(x, y);
    puts("\xC9", 0);
    for (int i = 0; i < strlen(text) + 2; i++)
    {
        puts("\xCD", 0);
    }
    puts("\xBB", 1);
    set_cursor(x, y + 1);
    puts("\xBA", 0);
    puts(" ", 0);
    puts(text, 0);
    puts(" ", 0);
    puts("\xBA", 1);
    set_cursor(x, y + 2);
    puts("\xC8", 0);
    for (int i = 0; i < strlen(text) + 2; i++)
    {
        puts("\xCD", 0);
    }
    puts("\xBC", 1);
}

void print_multiline(char *text, int x, int y, int w, int h)
{
    int cx = 0;
    int cy = 0;
    set_cursor(x, y);
    for (int i = 0; text[i] != '\0'; i++)
    {
        if (text[i] == '\n')
        {
            cx = 0;
            cy++;
        }
        else
        {
            putc(text[i]);
            cx++;
        }
        if (cx >= w)
        {
            cx = 0;
            cy++;
        }
        if (cy >= h)
        {
            break;
        }
        set_cursor(x + cx, y + cy);
    }
}

void set_color(int fg, int bg)
{
    current_color = fg | (bg << 4);
    current_fgc = fg;
    current_bgc = bg;
}

void set_cursor(int x, int y)
{
    cursor_x = x;
    cursor_y = y;
}

void cls()
{
    for (int i = 0; i < ROWS * COLUMNS * 2; i += 2)
    {
        video_fb[i] = ' ';
        video_fb[i + 1] = current_color;
    }
}

void disable_vga_blink()
{
    inb(0x3DA);
    outb(0x3C0, 0x10 | 0x20);
    int config = inb(0x3C1);
    config &= ~(1 << 3);
    outb(0x3C0, config);
}

void disable_cursor()
{
    outb(0x3D4, 0x0A);
    unsigned char val = inb(0x3D5);
    val |= 0x20;
    outb(0x3D5, val);
}

void enable_cursor(unsigned char start, unsigned char end)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, start & 0x1F);
    outb(0x3D4, 0x0B);
    outb(0x3D5, end & 0x1F);
}
