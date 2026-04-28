#ifndef IO_H
#define IO_H
#include <stdint.h>

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

void outb(unsigned short port, unsigned char val);
unsigned char inb(unsigned short port);
void outw(unsigned short port, unsigned short val);
uint16_t inw(unsigned short port);
unsigned char read_cmos(unsigned char reg);

#endif
