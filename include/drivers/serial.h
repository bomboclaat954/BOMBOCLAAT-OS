#ifndef SERIAL_H
#define SERIAL_H
#define COM1 0x3F8

void putc_serial(char a);
char read_serial();
int init_serial();

#endif
