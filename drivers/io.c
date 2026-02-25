#include "../include/io.h"

void outb(unsigned short port, unsigned char val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char inb(unsigned short port)
{
    unsigned char val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void outw(unsigned short port, unsigned short val)
{
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char read_cmos(unsigned char reg)
{
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}
