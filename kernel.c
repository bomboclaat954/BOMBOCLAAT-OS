// BOMBOCLAAT-OS KERNEL
#include "include/keyboard.h"
#include "include/io.h"
#include "include/string.h"
#include "include/screen.h"
#include "include/gui.h"
#include "include/api.h"
#include "include/calc.h"

#define COLUMNS 80
#define ROWS 25
#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
#define PIT_FREQ 1193182

char *prompt = "$ ";
char *last_cmd;
char *VER = "BOMBOCLAAT-OS 1.1";

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
#define PIT_FREQUENCY 1193182

int is_update_in_progress()
{
    outb(CMOS_ADDRESS, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

int bcd_to_bin(unsigned char bcd)
{
    return ((bcd / 16) * 10) + (bcd & 0xf);
}

char *datetime(int type)
{
    static char buf[16];
    while (is_update_in_progress())
        ;

    unsigned char second = bcd_to_bin(read_cmos(0x00));
    unsigned char minute = bcd_to_bin(read_cmos(0x02));
    unsigned char hour = bcd_to_bin(read_cmos(0x04));
    unsigned char day = bcd_to_bin(read_cmos(0x07));
    unsigned char month = bcd_to_bin(read_cmos(0x08));
    unsigned char year = bcd_to_bin(read_cmos(0x09));

    if (type == 0)
    { // HH:MM:SS
        buf[0] = (hour / 10) + '0';
        buf[1] = (hour % 10) + '0';
        buf[2] = ':';
        buf[3] = (minute / 10) + '0';
        buf[4] = (minute % 10) + '0';
        buf[5] = ':';
        buf[6] = (second / 10) + '0';
        buf[7] = (second % 10) + '0';
        buf[8] = '\0';
    }
    else if (type == 1)
    { // DD/MM/YY
        buf[0] = (day / 10) + '0';
        buf[1] = (day % 10) + '0';
        buf[2] = '/';
        buf[3] = (month / 10) + '0';
        buf[4] = (month % 10) + '0';
        buf[5] = '/';
        buf[6] = (year / 10) + '0';
        buf[7] = (year % 10) + '0';
        buf[8] = '\0';
    }
    else
    {
        buf[0] = (hour / 10) + '0';
        buf[1] = (hour % 10) + '0';
        buf[2] = ':';
        buf[3] = (minute / 10) + '0';
        buf[4] = (minute % 10) + '0';
        buf[5] = '\0';
    }
    return buf;
}

void reboot()
{
    unsigned char good = 0x02;
    while (good & 0x02)
    {
        good = inb(0x64);
    }
    outb(0x64, 0xFE);
}

void shutdown()
{
    outw(0xB004, 0x2000); // QEMU
    outw(0x604, 0x2000);  // QEMU / Bochs
    outw(0x4004, 0x3400); // VirtualBox
    __asm__ volatile("cli; hlt");
}

void update_clock()
{
    int old_x = cursor_x;
    int old_y = cursor_y;
    cursor_x = COLUMNS - 8;
    cursor_y = 0;
    puts(datetime(1), 0);
    cursor_x = COLUMNS - 6;
    cursor_y = 1;
    puts(datetime(2), 0);
    cursor_x = old_x;
    cursor_y = old_y;
    update_hardware_cursor();
}

void draw_main_screen(void)
{
    box(0, 0, VER);
    update_clock();
}

void get_cpu_model(char *model)
{
    unsigned int eax, ebx, ecx, edx;
    unsigned int *ptr = (unsigned int *)model;
    for (unsigned int i = 0; i < 3; i++)
    {
        unsigned int function = 0x80000002 + i;
        __asm__ volatile("cpuid"
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(function));
        ptr[i * 4 + 0] = eax;
        ptr[i * 4 + 1] = ebx;
        ptr[i * 4 + 2] = ecx;
        ptr[i * 4 + 3] = edx;
    }
    model[48] = '\0';
}

void info_gui()
{
    char cpu[49];
    get_cpu_model(cpu);
    char sysinfo_l1[128];
    char sysinfo_l2[128];
    char sysinfo[256];
    join(VER, " by BOMBOCLAAT.", sysinfo_l1, 1);
    join("CPU: ", cpu, sysinfo_l2, 0);
    join(sysinfo_l1, sysinfo_l2, sysinfo, 0);
    window("INFORMATIONS", sysinfo, 50, 2, 13, 10, 0x0F, 0x00, draw_menu);
}

void execute_command(char *cmd_line)
{
    last_cmd = cmd_line;
    putc('\n');
    char cmd[32];
    char arg[96];
    int i = 0, j = 0;
    while (cmd_line[i] != ' ' && cmd_line[i] != '\0' && i < 31)
    {
        cmd[i] = cmd_line[i];
        i++;
    }
    cmd[i] = '\0';

    while (cmd_line[i] == ' ')
        i++;

    while (cmd_line[i] != '\0' && j < 95)
    {
        arg[j] = cmd_line[i];
        i++;
        j++;
    }
    arg[j] = '\0';

    to_lower_case(cmd);
    to_lower_case(arg);

    if (strcmp(cmd, "help") == 0)
    {
        puts("Available commands:", 1);
        puts("cls               - clear screen", 1);
        puts("info              - informations about this system and computer", 1);
        puts("box <text>        - show a box with yout text", 1);
        puts("time              - show current time (HH:MM:SS)", 1);
        puts("date              - show current date (DD.MM.YY)", 1);
        puts("reboot            - reboot your computer", 1);
        puts("shutdown, exit    - shut down your computer", 1);
        puts("color <color>     - change text color", 1);
        puts("gui               - graphic interface", 1);
    }
    else if (strcmp(cmd, "cls") == 0)
    {
        cls();
        box(0, 0, VER);
        update_clock();
    }
    else if (strcmp(cmd, "info") == 0)
    {
        puts(VER, 0);
        puts(" by BOMBOCLAAT.", 1);
        char cpu[49];
        get_cpu_model(cpu);
        puts("Processor: ", 0);
        puts(cpu, 1);
    }
    else if (strcmp(cmd, "box") == 0)
    {
        if (strlen(arg) > 0)
        {
            box(cursor_x, cursor_y, arg);
        }
        else
        {
            puts("Usage: box <your text>", 1);
        }
    }
    else if (strcmp(cmd, "time") == 0)
    {
        puts(datetime(0), 1);
    }
    else if (strcmp(cmd, "date") == 0)
    {
        puts(datetime(1), 1);
    }
    else if (strcmp(cmd, "reboot") == 0)
    {
        puts("Rebooting...", 0);
        reboot();
    }
    else if (strcmp(cmd, "shutdown") == 0 || strcmp(cmd, "exit") == 0)
    {
        puts("Shutting down...", 0);
        shutdown();
    }
    else if (strcmp(cmd, "color") == 0)
    {
        if (strlen(arg) > 0)
        {
            set_color(0x03, 0x00);
            if (strcmp(arg, "blue") == 0)
            {
                set_color(0x01, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "green") == 0)
            {
                set_color(0x02, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "cyan") == 0)
            {
                set_color(0x03, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "red") == 0)
            {
                set_color(0x04, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "magenta") == 0)
            {
                set_color(0x04, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "orange") == 0)
            {
                set_color(0x06, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "default") == 0)
            {
                set_color(0x07, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "dark_gray") == 0)
            {
                set_color(0x08, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "light_blue") == 0)
            {
                set_color(0x09, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "light_green") == 0)
            {
                set_color(0x0A, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "light_cyan") == 0)
            {
                set_color(0x0B, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "light_red") == 0)
            {
                set_color(0x0C, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "light_magenta") == 0)
            {
                set_color(0x0D, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "yellow") == 0)
            {
                set_color(0x0E, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strcmp(arg, "white") == 0)
            {
                set_color(0x0F, 0x00);
                cls();
                draw_main_screen();
            }
            else if (strlen(arg) > 0)
            {
                set_color(0x07, 0x00);
                puts("Unknown color: ", 0);
                puts(arg, 1);
            }
        }
        else
        {
            puts("Usage: color <color>", 1);
            set_color(0x01, 0x00);
            puts("blue", 1);
            set_color(0x02, 0x00);
            puts("green", 1);
            set_color(0x03, 0x00);
            puts("cyan", 1);
            set_color(0x04, 0x00);
            puts("red", 1);
            set_color(0x05, 0x00);
            puts("magenta", 1);
            set_color(0x06, 0x00);
            puts("orange", 1);
            set_color(0x07, 0x00);
            puts("default", 1);
            set_color(0x08, 0x00);
            puts("dark_gray", 1);
            set_color(0x09, 0x00);
            puts("light_blue", 1);
            set_color(0x0A, 0x00);
            puts("light_green", 1);
            set_color(0x0B, 0x00);
            puts("light_cyan", 1);
            set_color(0x0C, 0x00);
            puts("light_red", 1);
            set_color(0x0D, 0x00);
            puts("light_magenta", 1);
            set_color(0x0E, 0x00);
            puts("yellow", 1);
            set_color(0x0F, 0x00);
            puts("white", 1);
            set_color(0x07, 0x00);
        }
    }
    else if (strcmp(cmd, "gui") == 0)
    {
        char *opts[] = {"Shut down", "Reboot", "Info", "Exit"};
        void (*actions[])(void) = {shutdown, reboot, info_gui, return_to_kernel};
        menu(opts, 4, actions);
    }
    else if (strcmp(cmd, "calc") == 0)
    {
        calc_main(arg);
    }
    else if (strlen(cmd) > 0)
    {
        puts("Unknown command: ", 0);
        puts(cmd, 1);
    }
    puts(prompt, 0);
}

void kernel_main(void)
{
    char command_buffer[128];
    int buf_idx = 0;
    int last_minute = -1;
    int extended = 0;

    puts(prompt, 0);

    while (1)
    {
        // CLOCK UPDATE
        if (!is_update_in_progress())
        {
            int current_minute = bcd_to_bin(read_cmos(0x02));
            if (current_minute != last_minute)
            {
                update_clock();
                last_minute = current_minute;
            }
        }
        // COMMANDS
        if (inb(0x64) & 1)
        {
            unsigned char scancode = inb(0x60);
            if (!(scancode & 0x80))
            {
                if (scancode == 0x2A || scancode == 0x36)
                {
                    shift_pressed = 1;
                }
                else if (scancode == 0x3A)
                {
                    caps_lock = !caps_lock;
                }
                else
                {
                    char c = get_ascii(scancode);
                    if (c == '\n')
                    {
                        command_buffer[buf_idx] = '\0';
                        execute_command(command_buffer);
                        buf_idx = 0;
                    }
                    else if (c == '\b')
                    {
                        if (buf_idx > 0)
                        {
                            buf_idx--;
                            putc('\b');
                        }
                    }
                    else if (c > 0 && buf_idx < 127)
                    {
                        command_buffer[buf_idx++] = c;
                        putc(c);
                    }
                }
            }
            else
            {
                unsigned char released_code = scancode & 0x7F;
                if (released_code == 0x2A || released_code == 0x36)
                {
                    shift_pressed = 0;
                }
            }
        }
    }
}

void start_kernel(long magic)
{
    if (magic != 0x2BADB002)
    {
        char *m;
        set_color(0x04, 0x00);
        itoa(magic, m, 16);
        puts("KERNEL PANIC!", 1);
        puts("Invalid multiboot signature: ", 0);
        puts(m, 1);
        puts("Press ESC to shut down or ENTER to reboot", 1);
        while (1)
        {
            if (inb(0x64) & 1)
            {
                int scancode = inb(0x60);
                if (scancode == 0x01)
                {
                    shutdown();
                }
                else if (scancode == 0x1C)
                {
                    reboot();
                }
            }
        }
    }
    update_hardware_cursor();
    box(0, 0, VER);
    set_cursor(0, 3);
    puts("Type ", 0);
    set_color(0x0F, 0x00);
    puts("help", 0);
    set_color(0x07, 0x00);
    puts(" for commands list", 2);
    // puts("Type help for commands list", 2);
    kernel_main();
}

void return_to_kernel(void)
{
    enable_cursor(0x0E, 0x0F);
    set_color(0x07, 0x00);
    cls();
    box(0, 0, VER);
    kernel_main();
}
