#include "../include/gui.h"
#include "../include/screen.h"
#include "../include/io.h"
#include "../include/api.h"
#include "../include/string.h"

char **options_global;
int count_global;
int index = 0;
int old;

void update_selection(char *options[], int old, int now)
{
    set_cursor(18, 4 + old);
    putc(' ');
    set_cursor(18, 4 + now);
    // putc('\x1A');
    putc('*');
}

void draw_menu(char *options[], int count)
{
    cls();
    set_color(0x03, 0x00);
    set_cursor(0, 0);
    for (int i = 0; i < COLUMNS; i++)
    {
        putc('\xDB');
    }
    set_cursor(0, 0);
    set_color(0x00, 0x03);
    puts(VER, 1);
    set_color(0x07, 0x00);
    for (int i = 0; i < COLUMNS; i++)
    {
        for (int j = 0; j < ROWS - 2; j++)
        {
            putc('\xDB');
        }
    }
    set_color(0x03, 0x00);
    set_cursor(0, ROWS - 1);
    for (int i = 0; i < COLUMNS; i++)
    {
        putc('\xDB');
    }
    set_cursor(17, 3);
    set_color(0x0F, 0x0F);
    for (int i = 0; i < 20; i++)
    {
        for (int j = 0; j < 45; j++)
        {
            putc('\xDB');
        }
        set_cursor(17, i + 3);
    }
    set_cursor(0, 1);
    set_color(0x00, 0x0F);
    for (int i = 0; i < count; i++)
    {
        set_cursor(20, 4 + i);
        puts(options[i], 1);
    }
    update_selection(options, 1, 0);
}

void update_clock_gui()
{
    int last_x, last_y, last_fgc, last_bgc;
    last_x = cursor_x;
    last_y = cursor_y;
    last_fgc = current_fgc;
    last_bgc = current_bgc;
    set_cursor(0, ROWS - 1);
    set_color(0x00, 0x03);
    puts(datetime(1), 0);
    puts(" ", 0);
    puts(datetime(2), 1);
    set_color(last_fgc, last_bgc);
}

void window(char *title, char *text, int w, int h, int x, int y, int bgc, int fgc, void (*after)(char *[], int))
{
    // ╔ = \xC9; ═ = \xCD; ╗ = \xBB; ║ = \xBA; ╚ = \xC8; ╝ = \0xBC
    set_color(fgc, bgc);
    set_cursor(x, y);
    puts("\xC9", 0);
    puts(title, 0);
    for (int i = 0; i < w - strlen(title); i++)
    {
        puts("\xCD", 0);
    }
    puts("\xBB", 1);
    set_cursor(x, y + 1);
    for (int i = 0; i < h; i++)
    {
        set_cursor(x, y + 1 + i);
        puts("\xBA", 0);
        for (int j = 0; j < w; j++)
            putc(' ');
        puts("\xBA", 1);
    }
    set_cursor(x, y + 1 + h);
    puts("\xC8", 0);
    for (int i = 0; i < w; i++)
        puts("\xCD", 0);
    puts("\xBC", 0);
    print_multiline(text, x + 1, y + 1, w, h);
    puts(" ", 0);
    disable_cursor();
    set_color(0x00, 0x0F);
    set_cursor(x + 1, y + h + 2);
    for (int i = 0; i < w + 2; i++)
        putc('\xDB');
    for (int i = 0; i < h + 1; i++)
    {
        set_cursor(cursor_x - 1, y + h - i + 1);
        putc('\xDB');
    }
    while (1)
    {
        if (inb(0x64) & 1)
        {
            unsigned char sc = inb(0x60);
            if (sc == 0x1C)
            {
                after(options_global, count_global);
                update_selection(options_global, 0, 2);
                update_clock_gui();
                break;
            }
        }
    }
}

void menu(char *options[], int count, void (*actions[])(void))
{
    int last_minute = -1;
    update_clock_gui();
    count_global = count;
    options_global = options;
    disable_vga_blink();
    draw_menu(options, count);
    while (1)
    {
        if (!is_update_in_progress())
        {
            int current_minute = bcd_to_bin(read_cmos(0x02));
            if (current_minute != last_minute)
            {
                update_clock_gui();
                last_minute = current_minute;
            }
        }
        if (inb(0x64) & 1)
        {
            static int extended = 0;
            int scancode = inb(0x60);
            if (scancode == 0xE0)
            {
                extended = 1;
                continue;
            }
            else if (scancode & 0x80)
                continue;
            else if (scancode == 0x1C)
                actions[index]();
            else if (scancode == 0x01)
                return_to_kernel();
            if (extended)
            {
                extended = 0;
                if (scancode == 0x48)
                {
                    old = index;
                    if (index > 0)
                        index--;
                    else
                        index = count - 1;
                    update_selection(options, old, index);
                }
                else if (scancode == 0x50)
                {
                    old = index;
                    if (index < count - 1)
                        index++;
                    else
                        index = 0;
                    update_selection(options, old, index);
                }
            }
        }
    }
}
