/*
    BOMBOCLAAT-OS MUSIC LIBRARY
*/
#include <lib/music.h>
#include <drivers/io.h>
#include <int/int.h>

void tone(uint32_t freq)
{
    uint32_t div;
    uint8_t tmp;

    div = 1193180 / freq;
    outb(0x43, 0xb6);
    outb(0x42, (uint8_t)(div));
    outb(0x42, (uint8_t)(div >> 8));

    tmp = inb(0x61);

    if (!(tmp & 3))
    {
        outb(0x61, tmp | 3);
    }
}

void noTone()
{
    uint8_t tmp = inb(0x61) & 0xFC;

    outb(0x61, tmp);
}

void play_song(melody_t *m, int len, int bpm)
{
    int wholenote = (60000 / bpm) * 4;

    for (int i = 0; i < len; i++)
    {
        if (m[i].note == -1)
        {
            noTone();
            delay_ms(wholenote / m[i].note);
        }
        else
        {
            tone(m[i].freq);
            delay_ms(wholenote / m[i].note);
            noTone();
        }
    }
}
