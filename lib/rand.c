#include <lib/rand.h>

uint32_t rand_state = 1;

uint32_t rand()
{
    rand_state = rand_state * 1664525 + 1013904223;
    return rand_state;
}

void srand(uint32_t seed)
{
    rand_state = seed;
}

void random_seed()
{
    uint32_t seed;
    __asm__ volatile("rdtsc" : "=a"(seed));
    srand(seed);
}
