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

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#define MULTIBOOT_INFO_MEM_MAP 0x40

char *prompt = "$ ";
char *VER = "BOMBOCLAAT-OS 1.6b1";
char RAM_MB[10];

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

void panic(char *msg)
{
    set_color(0x04, 0x00);
    puts("KERNEL PANIC!", 1);
    puts("Reason: ", 0);
    puts(msg, 1);
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
        panic("stack overflow");
    }
}

void cause_stack_overflow(int depth)
{
    check_stack();
    if (depth % 10 == 0)
    {
        char tmp[16];
        itoa(depth, tmp, 10);
        puts("Depth: ", 0);
        puts(tmp, 1);
    }
    cause_stack_overflow(depth + 1);
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
        puts("source                - get informations about this OS source code", 1);
        puts("panic <msg>           - makes a kernel panic with your message", 1);
        puts("bootable              - check if installed disk has boot signature (0xAA55)", 1);
        puts("overflow              - cause stack overflow and see how deep it can get", 1);
        puts("erase_sector <lba>    - erases all data on a sector", 1);
        puts("read_sector <lba>     - reads and prints all data from a sector", 1);
        puts("(beta) timer <m:s>    - sets a countdown timer to <m> minutes and <s> seconds", 1);
        puts("beep <freq>           - beeps with provided frequency for 1s", 1);
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
        }
    }
    else if (strcmp(cmd, "updates") == 0)
    {
        puts("Last update date: 3/04/2026", 1);
        puts("What's new:", 1);
        puts("  - added interrupt handling", 1);
        puts("  - added speaker support", 1);
        puts("  - new commands: timer (beta), beep, song", 1);
        puts("  - changed includes in the code to <>", 1);
        puts("  - removed unused files from the code", 1);
        puts("This is the biggest and the most important update so far", 1);
    }
    else if (strcmp(cmd, "source") == 0)
        puts("Get the source code at: https://www.github.com/bomboclaat954/bomboclaat-os", 1);

    else if (strcmp(cmd, "panic") == 0)
    {
        if (strlen(arg) > 0)
            panic(arg);
        else
            panic("no reason");
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
    else if (strcmp(cmd, "bootable") == 0)
    {
        uint16_t data[256];
        ata_read_sector(0, data);
        if (data[255] == 0xAA55)
            puts("Disk is bootable", 1);
        else
            puts("Disk is not bootable", 1);
    }
    else if (strcmp(cmd, "overflow") == 0)
        cause_stack_overflow(1);
    else if (strcmp(cmd, "erase_sector") == 0)
    {
        if (strlen(arg) > 0)
        {
            uint16_t tmp = atoi(arg);
            puts("Are you sure? All data on sector ", 0);
            puts(arg, 0);
            puts(" will be destroyed", 1);
            puts("ENTER - YES, ESC - NO", 1);
            while (1)
            {
                if (inb(0x64) & 1)
                {
                    unsigned char sc = inb(0x60);
                    if (sc == 0x1C)
                    {
                        puts("Erasing...", 1);
                        ata_erase_sector(tmp);
                        puts("Done", 1);
                        break;
                    }
                    else if (sc == 0x01)
                    {
                        puts("Abort", 1);
                        break;
                    }
                }
            }
        }
        else
            puts("Error: no sector number specified", 1);
    }
    else if (strcmp(cmd, "read_sector") == 0)
    {
        if (strlen(arg) > 0)
        {
            uint16_t arg_ = atoi(arg);
            uint16_t buf[256];
            ata_read_sector(arg_, buf);
            char *tmp = "";
            for (int i = 0; i < 256; i++)
            {
                tmp = "";
                itoa(reverse_endian(buf[i]), tmp, 16);
                if (strlen(tmp) == 1)
                    tmp = join("000", tmp, tmp, 0);
                else if (strlen(tmp) == 2)
                    tmp = join("00", tmp, tmp, 0);
                else if (strlen(tmp) == 3)
                    tmp = join("0", tmp, tmp, 0);
                puts(tmp, 0);
                puts(" ", 0);
            }
        }
        else
            puts("Error: no sector number specified", 1);
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
            _s = ms / 1000;
            _m = 0;
            while (ms)
            {
                if (_s >= 60)
                {
                    _s -= 60;
                    _m++;
                }
                if (ms % 1000 == 0)
                {
                    cls();
                    draw_main_screen();
                    if (_s != 0)
                        _s--;
                    itoa(_m, buf, 10);
                    puts(buf, 0);
                    clear_str(buf);
                    puts(":", 0);
                    itoa(_s, buf, 10);
                    puts(buf, 0);
                }
                else if (_s % 60 == 0)
                {
                    if (_m != 0)
                        _m--;
                }
                delay_ms(1);
                ms--;
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
        melody_t m[] = {
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
        play_song(m, ARRAY_SIZE(m), 116);
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

void start_kernel(long magic, uint32_t mboot_info_addr)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        panic("invalid multiboot signature");

    update_hardware_cursor();
    box(0, 0, VER);
    set_cursor(0, 3);

    multiboot_info_t *mbi = (multiboot_info_t *)mboot_info_addr;
    pmm_init(mbi);
    uint64_t ram = multiboot_get_ram(mbi, 2); // RAM size in MB
    itoa(ram, RAM_MB, 10);
    if (ram == 0)
        panic("error while getting RAM amount");
    else if (ram < 64)
        panic("too little RAM");

    puts("Type ", 0);
    set_color(0x0F, 0x00);
    puts("help", 0);
    set_color(0x07, 0x00);
    puts(" for commands list", 2);

    // init interrupts and timer
    idt_init();
    pit_init();

    // FPU and SSE are for better operations on floats
    fpu_enable();
    sse_enable();

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
