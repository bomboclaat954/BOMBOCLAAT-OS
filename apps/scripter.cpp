#include <apps/scripter.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/io.h>
#include <bomboclaat-os/kprintf.h>
#include <bomboclaat-os/api.h>
#include <lib/string.h>
#include <memory/kmalloc.h>

void scripter_help()
{
    cls();
    set_cursor(0, 0);
    kprintf("%aSCRIPTER 0.0.1-alpha                                                ESC - return", 0x07);
    kprintf("%a", 0x70);
    kprintf("OUT <text> - write text\n");
    kprintf("BOX <text> - write text in box\n");
    kprintf("DLY <ms>   - delay\n");
    while (!(input_key() == 0x01))
        ;
    scripter_main();
}

void input_scripter(char *buf)
{
    int buf_idx = 0;
    int lines = 1;
    kprintf("00%d\xBA ", lines);

    while (1)
    {
        if (inb(0x64) & 1)
        {
            unsigned char scancode = inb(0x60);
            if (!(scancode & 0x80))
            {
                if (scancode == 0x2A || scancode == 0x36)
                    shift_pressed = 1;
                else if (scancode == 0x3A)
                    caps_lock = !caps_lock;
                else if (scancode == 0x58)
                {
                    break;
                    return;
                }
                else if (scancode == 0x57)
                    scripter_help();
                else
                {
                    char c = get_ascii(scancode);
                    if (c == '\n')
                    {
                        buf[buf_idx++] = '\n';
                        putc('\n');
                        lines++;
                        if (lines < 10)
                            kprintf("00%d\xBA ", lines);
                        else if (lines >= 10 && lines < 100)
                            kprintf("0%d\xBA ", lines);
                        else if (lines >= 100)
                            kprintf("%d\xBA ", lines);
                    }
                    else if (c == '\b')
                    {
                        if (buf_idx > 0)
                        {
                            buf_idx--;
                            putc('\b');
                        }
                    }
                    else /*if (c > 0 && buf_idx < 127 && buf_idx)*/
                    {
                        buf[buf_idx++] = c;
                        putc(c);
                    }
                }
            }
            else
            {
                unsigned char released_code = scancode & 0x7F;
                if (released_code == 0x2A || released_code == 0x36)
                    shift_pressed = 0;
            }
        }
    }
}

void execute_script(char *buf)
{
    cls();
    draw_main_screen();

    char *line = buf;
    char *next_line;

    while (line != NULL && *line != '\0')
    {
        next_line = strchr(line, '\n');
        if (next_line)
            *next_line = '\0';
        char *txt = line + 3;
        if (*txt == ' ')
            txt++;
        if (strncmp(line, "OUT", 3) == 0)
            kprintf("%s\n", txt);

        else if (strncmp(line, "BOX", 3) == 0)
            box(cursor_x, cursor_y, txt);

        else if (strncmp(line, "DLY", 3) == 0)
        {
            int ms = atoi(txt);
            delay_ms(ms);
        }

        if (next_line)
        {
            *next_line = '\n';
            line = next_line + 1;
        }
        else
            line = NULL;
    }
}

void scripter_main()
{
    cls();
    set_cursor(0, 0);
    kprintf("%aSCRIPTER 0.0.1-alpha                                F11 - help; F12 - run script", 0x07);
    kprintf("%a", 0x70);
    char *buf = (char *)kmalloc(65536 * sizeof(char *)); // reserve 64kB for the script
    input_scripter(buf);
    execute_script(buf);
    kfree(buf);
}
