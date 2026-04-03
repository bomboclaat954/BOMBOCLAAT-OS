#include <int.h>
#include <keyboard.h>
#include <io.h>

void irq_handler(registers_t *r)
{
    if (r->int_no == 32)
        pit_tick();
    if (r->int_no >= 40)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
}
