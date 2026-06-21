/*
 * BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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
