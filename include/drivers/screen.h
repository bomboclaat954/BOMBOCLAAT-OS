#ifndef SCREEN_H
#define SCREEN_H

#ifdef __cplusplus
extern "C"
{
#endif

#define COLUMNS 80
#define ROWS 25

    extern int cursor_x;
    extern int cursor_y;
    extern char *video_fb;
    extern int current_color;
    extern int current_fgc;
    extern int current_bgc;

    void putc(char c);
    void puts(char *s, int endl);
    void info(char *s);
    void update_hardware_cursor();
    void scroll();
    void box(int x, int y, char *text);
    void set_color(int fg, int bg);
    void set_cursor(int x, int y);
    void cls();
    void disable_vga_blink();
    void disable_cursor();
    void enable_cursor(unsigned char start, unsigned char end);
    void load_font(unsigned char *font);

#ifdef __cplusplus
}
#endif

#endif
