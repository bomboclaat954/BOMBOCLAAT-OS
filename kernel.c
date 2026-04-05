/*
    BOMBOCLAAT-OS KERNEL
    The most important file of the OS
*/
#include <stdint.h>
#include <keyboard.h>
#include <io.h>
#include <string.h>
#include <screen.h>
#include <api.h>
#include <calc.h>
#include <disk.h>
#include <fat32.h>
#include <ram.h>
#include <int.h>
#include <music.h>
#include <math.h>
#include <bfs.h>
#include <rand.h>
#include <diskman.h>

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#define MULTIBOOT_INFO_MEM_MAP 0x40

char *prompt = "$ ";
char *VER = "BOMBOCLAAT-OS 1.7";
char RAM_MB[10];
char letters_digits[37] = "QWERTYUIOPASDFGHJKLZXCVBNM0123456789";

extern uint32_t stack_guard;

uint16_t reverse_endian(uint16_t nb)
{
    return (nb >> 8) | (nb << 8);
}

void fpu_enable()
{
    unsigned int cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2);
    cr0 |= (1 << 1);
    asm volatile("mov %0, %%cr0" ::"r"(cr0));
    asm volatile("fninit");
}

void sse_enable()
{
    unsigned int cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (3 << 9);
    asm volatile("mov %0, %%cr4" ::"r"(cr4));
}

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

void reg_dump(registers_t *r)
{
    __asm__ volatile(
        "mov %%eax, %0\n"
        "mov %%ebx, %1\n"
        "mov %%ecx, %2\n"
        "mov %%edx, %3\n"
        "mov %%esp, %4\n"
        "mov %%ebp, %5\n"
        "mov %%esi, %6\n"
        "mov %%edi, %7\n"
        : "=m"(r->eax), "=m"(r->ebx), "=m"(r->ecx), "=m"(r->edx),
          "=m"(r->esp), "=m"(r->ebp), "=m"(r->esi), "=m"(r->edi));

    __asm__ volatile(
        "call 1f\n"
        "1: pop %0\n"
        : "=r"(r->eip));

    __asm__ volatile(
        "mov %%cs, %0\n"
        "mov %%ds, %1\n"
        "mov %%es, %2\n"
        "mov %%fs, %3\n"
        "mov %%gs, %4\n"
        : "=m"(r->cs), "=m"(r->ds), "=m"(r->es), "=m"(r->fs), "=m"(r->gs));
}

void panic(char *msg, registers_t *r, int from_cpu)
{
    set_color(0x04, 0x00);
    puts("*** KERNEL PANIC ***", 1);
    puts("Reason: ", 0);
    puts(msg, 1);

    if (!r)
        reg_dump(r);

    char buf[16];
    puts("EIP: 0x", 0);
    itoa(r->eip, buf, 16);
    puts(buf, 1);
    puts("ESP: 0x", 0);
    itoa(r->esp, buf, 16);
    puts(buf, 1);
    puts("EAX: 0x", 0);
    itoa(r->eax, buf, 16);
    puts(buf, 1);
    if (from_cpu)
    {
        puts("ERR: 0x", 0);
        itoa(r->err_code, buf, 16);
        puts(buf, 1);
        puts("CS:  0x", 0);
        itoa(r->cs, buf, 16);
        puts(buf, 1);
    }
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

void check_stack()
{
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    if (esp <= (uint32_t)&stack_guard || (esp - (uint32_t)&stack_guard) < 512)
    {
        registers_t *r;
        panic("stack overflow", NULL, 0);
    }
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

static inline void cpuid(uint32_t leaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
    __asm__ volatile("cpuid"
                     : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
                     : "a"(leaf));
}

void get_cpu_model(char *model)
{
    unsigned int eax, ebx, ecx, edx;
    unsigned int *ptr = (unsigned int *)model;
    for (unsigned int i = 0; i < 3; i++)
    {
        unsigned int function = 0x80000002 + i;
        cpuid(function, &eax, &ebx, &ecx, &edx);
        ptr[i * 4 + 0] = eax;
        ptr[i * 4 + 1] = ebx;
        ptr[i * 4 + 2] = ecx;
        ptr[i * 4 + 3] = edx;
    }
    model[48] = '\0';
}

void execute_command(char *cmd_line)
{
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

    if (strcmp(cmd, "help") == 0)
    {
        puts("Available commands:", 1);
        puts("cls                   - clear commands output", 1);
        puts("info                  - informations about software and hardware", 1);
        puts("box <text>            - show a box with yout text", 1);
        puts("time                  - show current time (HH:MM:SS)", 1);
        puts("date                  - show current date (DD.MM.YY)", 1);
        puts("reboot                - reboot your computer", 1);
        puts("shutdown, exit        - shut down your computer", 1);
        puts("color <color>         - change text color", 1);
        puts("updates               - check what's new in BOMBOCLAAT-OS", 1);
        puts("panic <msg>           - makes a kernel panic with your message", 1);
        puts("timer <m:s>           - sets a countdown timer to <m> minutes and <s> seconds", 1);
        puts("beep <freq>           - beeps with provided frequency for 1s", 1);
        puts("song                  - play an example song", 1);
        puts("diskman <opt>         - disk manager (type diskman for option list)", 1);
    }
    else if (strcmp(cmd, "cls") == 0)
    {
        cls();
        box(0, 0, VER);
        update_clock();
    }
    else if (strcmp(cmd, "box") == 0)
    {
        if (strlen(arg) > 0)
            box(cursor_x, cursor_y, arg);
        else
            puts("Usage: box <your text>", 1);
    }
    else if (strcmp(cmd, "time") == 0)
        puts(datetime(0), 1);
    else if (strcmp(cmd, "date") == 0)
        puts(datetime(1), 1);
    else if (strcmp(cmd, "reboot") == 0)
        reboot();
    else if (strcmp(cmd, "shutdown") == 0 || strcmp(cmd, "exit") == 0)
        shutdown();
    else if (strcmp(cmd, "color") == 0)
    {
        if (strlen(arg) > 0)
        {
            static const struct
            {
                char *name;
                uint8_t code;
            } colors[] = {
                {"blue", 0x01}, {"green", 0x02}, {"cyan", 0x03}, {"red", 0x04}, {"magenta", 0x05}, {"orange", 0x06}, {"default", 0x07}, {"dark_gray", 0x08}, {"light_blue", 0x09}, {"light_green", 0x0A}, {"light_cyan", 0x0B}, {"light_red", 0x0C}, {"light_magenta", 0x0D}, {"yellow", 0x0E}, {"white", 0x0F}, {0, 0}};
            for (int i = 0; colors[i].name; i++)
            {
                if (strcmp(arg, colors[i].name) == 0)
                {
                    set_color(colors[i].code, 0x00);
                    cls();
                    draw_main_screen();
                }
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
    else if (strcmp(cmd, "calc") == 0)
    {
        if (strlen(arg) > 0)
            calc_main(arg);
        else
        {
            puts("How to use BOMBOCLAAT-OS calculator", 1);
            puts("Enter only one expression (like 2+2)", 1);
            puts("Supported operations:", 1);
            puts("a+b add", 1);
            puts("a-b subtract", 1);
            puts("a*b multiply", 1);
            puts("a/b divide", 1);
            puts("a^b power", 1);
            puts("a%b percent (a% of b)", 1);
            puts("$a  square root of a", 1);
        }
    }
    else if (strcmp(cmd, "updates") == 0)
    {
        puts("Last update date: 5/04/2026", 1);
        puts("What's new: ", 1);
        puts("  - added BFS (BOMBOCLAAT File System)", 1);
        puts("  - new command: diskman", 1);
        puts("  - deleted commands: read_sector, erase_sector, bootable", 1);
        puts("  - added random numbers generating", 1);
    }
    else if (strcmp(cmd, "panic") == 0)
    {
        if (strlen(arg) > 0)
            panic(arg, NULL, 0);
        else
            panic("no reason", NULL, 0);
    }
    else if (strcmp(cmd, "info") == 0)
    {
        puts("Software informations", 1);
        puts("Version: ", 0);
        puts(VER, 1);
        puts("", 1);
        puts("Hardware informations", 1);
        puts("CPU: ", 0);
        char cpu[49];
        get_cpu_model(cpu);
        puts(cpu, 1);
        puts("Total RAM: ", 0);
        puts(RAM_MB, 0);
        puts(" MB", 1);
        char used_ram[16];
        itoa(get_used_ram_kb(), used_ram, 10);
        puts("Used RAM: ", 0);
        puts(used_ram, 0);
        puts(" kB", 1);

        uint8_t disk_detected = detect_ata_drive(0);
        if (disk_detected == 0)
            puts("Disk not detected", 1);
        else
        {
            puts("Disk model: ", 0);
            char disk[64];
            get_drive_model(0, disk);
            puts(disk, 1);
            puts("Total disk capacity: ", 0);
            uint32_t sectors = get_ata_capacity_sectors(0);
            uint32_t size_mb = (sectors * 512) / (1024 * 1024);
            char size_str[16];
            itoa(size_mb, size_str, 10);
            puts(size_str, 0);
            puts(" MB", 1);
        }
    }
    else if (strcmp(cmd, "timer") == 0)
    {
        if (strlen(arg) > 0)
        {
            cls();
            draw_main_screen();
            char m[3], s[3];
            int _m, _s;
            char buf[4];
            if (arg[2] == ':')
            {

                m[0] = arg[0];
                m[1] = arg[1];
                s[0] = arg[3];
                s[1] = arg[4];
            }
            else
            {
                m[0] = arg[0];
                s[0] = arg[2];
                s[1] = arg[3];
            }
            m[2] = '\0';
            s[2] = '\0';
            _m = atoi(m);
            _s = atoi(s);

            int ms = (_m * 60000) + (_s * 1000);
            int total_s = ms / 1000;

            while (total_s >= 0)
            {
                int disp_m = total_s / 60;
                int disp_s = total_s % 60;

                cls();
                draw_main_screen();

                char buf[8];
                itoa(disp_m, buf, 10);
                puts(buf, 0);
                puts(":", 0);

                if (disp_s < 10)
                    puts("0", 0);
                itoa(disp_s, buf, 10);
                puts(buf, 0);

                if (total_s == 0)
                    break;

                delay_ms(1000);
                total_s--;
            }

            puts("", 1);
            puts("Time's up!", 1);
            tone(1000);
            delay_ms(500);
            noTone();
        }
        else
            puts("Error: time not specified or in bad format", 1);
    }
    else if (strcmp(cmd, "beep") == 0)
    {
        if (strlen(arg) > 0)
        {
            int freq = atoi(arg);
            tone(freq);
            delay_ms(1000);
            noTone();
        }
        else
            puts("Error: no frequency provided", 1);
    }
    else if (strcmp(cmd, "song") == 0)
    {
        // Example song: Eric Clapton - Layla (intro). 116 BPM, 4/4
        puts("Note: if you're using VirtualBox, you probably won't hear anything", 1);
        puts("\x0E Eric Clapton - Layla", 1);
        melody_t layla[] = {
            {NOTE_A4, 16},
            {NOTE_C5, 16},
            {NOTE_D5, 16},
            {NOTE_F5, 16},
            {NOTE_D5, 16},
            {NOTE_C5, 16},
            {NOTE_D5, 4},
            {REST, 8},
            {NOTE_G5, 4},
            {NOTE_F5, 4},
            {NOTE_E5, 4},
            {NOTE_C5, 4},
            {NOTE_D5, 4},
            {NOTE_A4, 16},
            {NOTE_C5, 16},
            {NOTE_D5, 16},
            {NOTE_F5, 16},
            {NOTE_D5, 16},
            {NOTE_C5, 16},
            {NOTE_D5, 4},
            {REST, 8},
            {NOTE_A5, 4},
            {NOTE_G5, 4},
            {NOTE_E5, 4},
            {NOTE_C5, 4},
            {NOTE_D5, 4},
            {NOTE_A4, 16},
            {NOTE_C5, 16},
            {NOTE_D5, 16},
            {NOTE_F5, 16},
            {NOTE_D5, 16},
            {NOTE_C5, 16},
            {NOTE_D5, 4},
            {REST, 8},
            {NOTE_G5, 4},
            {NOTE_F5, 4},
            {NOTE_E5, 4},
            {NOTE_C5, 4},
            {NOTE_D5, 4},
            {NOTE_A4, 16},
            {NOTE_C5, 16},
            {NOTE_D5, 16},
            {NOTE_F5, 16},
            {NOTE_D5, 16},
            {NOTE_C5, 16},
            {NOTE_D5, 4},
            {REST, 8},
            {NOTE_A5, 4},
            {NOTE_G5, 4},
            {NOTE_E5, 4},
            {NOTE_C5, 4},
            {NOTE_C5, 16},
            {NOTE_CS5, 2},
        };
        play_song(layla, ARRAY_SIZE(layla), 116);
    }
    else if (strcmp(cmd, "diskman") == 0)
    {
        if (arg > 0)
            diskman(arg);
        else
            diskman("h");
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
        // check if stack isn't overflowed
        check_stack();
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
            if (scancode == 0xE0)
            {
                extended = 1;
                continue;
            }
            if (!(scancode & 0x80))
            {
                /*if (extended) // 0x48 - UP, 0x50 - DOWN, 0x4B - LEFT, 0x4D - RIGHT
                {
                    // if(...) {...; continue;} <- REMEMBER ABOUT CONTINUE
                }*/
                if (scancode == 0x2A || scancode == 0x36)
                    shift_pressed = 1;
                else if (scancode == 0x3A)
                    caps_lock = !caps_lock;
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
                    shift_pressed = 0;
            }
        }
    }
}

void passwd(char *passwd)
{
    char buf[128];
    int buf_idx = 0;
    puts("Password: ", 0);
    while (1) // not using input() here because it does putc(c) instead of putc('*')
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
                else
                {
                    char c = get_ascii(scancode);
                    if (c == '\n')
                    {
                        buf[buf_idx] = '\0';
                        buf_idx = 0;
                        if (strcmp(buf, passwd) == 0)
                        {
                            puts("", 1);
                            puts("Welcome!", 1);
                            break;
                        }
                        else
                        {
                            puts("", 1);
                            puts("Incorrect password", 1);
                            puts("Password: ", 0);
                            continue;
                        }
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
                        buf[buf_idx++] = c;
                        putc('*');
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

void start_kernel(long magic, uint32_t mboot_info_addr)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        panic("invalid multiboot signature", NULL, 0);

    multiboot_info_t *mbi = (multiboot_info_t *)mboot_info_addr;
    pmm_init(mbi);
    uint64_t ram = multiboot_get_ram(mbi, 2); // RAM size in MB
    itoa(ram, RAM_MB, 10);
    if (ram == 0)
        panic("error while getting RAM amount", NULL, 0);
    else if (ram < 64)
        panic("too little RAM", NULL, 0);

    // init interrupts and timer
    idt_init();
    pit_init();

    // FPU and SSE are for better operations on floats
    fpu_enable();
    sse_enable();

    puts("Welcome to BOMBOCLAAT-OS!", 1);
    // passwd("BOMBOCLAAT");
    cls();
    update_hardware_cursor();
    box(0, 0, VER);
    set_cursor(0, 3);

    puts("Type ", 0);
    set_color(0x0F, 0x00);
    puts("help", 0);
    set_color(0x07, 0x00);
    puts(" for commands list", 2);

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
