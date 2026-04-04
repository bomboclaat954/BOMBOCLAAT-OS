#ifndef API_H
#define API_H
#include <int.h>

#define NULL ((void *)0)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
extern char *VER;

uint16_t reverse_endian(uint16_t nb);
void return_to_kernel(void);
int is_update_in_progress();
int bcd_to_bin(unsigned char bcd);
char *datetime(int type);
void panic(char *msg, registers_t *r, int from_cpu);

#endif
