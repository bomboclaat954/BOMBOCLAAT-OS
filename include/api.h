#ifndef API_H
#define API_H

extern char *VER;
void return_to_kernel(void);
int is_update_in_progress();
int bcd_to_bin(unsigned char bcd);
char *datetime(int type);

#endif
