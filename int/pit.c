#include <stdint.h>
#include <io.h>
#include <int.h>

#define PIT_CHANNEL0 0x40
#define PIT_CMD 0x43
#define PIT_FREQUENCY 1193182
#define PIT_HZ 1000

static volatile uint32_t ticks = 0;

void pit_init(void)
{
    uint16_t divisor = PIT_FREQUENCY / PIT_HZ;

    outb(PIT_CMD, 0x36);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8));

    uint8_t mask = inb(0x21);
    if (mask & 0x01)
        outb(0x21, mask & ~0x01);
}

void pit_tick(void)
{
    ticks++;
}

uint32_t pit_get_ticks(void)
{
    return ticks;
}

void delay_ms(uint32_t ms)
{
    uint32_t target = ticks + ms;
    while (ticks < target)
        ;
}
