#ifndef MENU_H
#define MENU_H

void update_selection(char *options[], int old, int now);
void draw_menu(char *options[], int count);
void window(char *title, char *text, int w, int h, int x, int y, int bgc, int fgc, void (*after)(char *[], int));
void menu(char *options[], int count, void (*actions[])(void));

#endif
