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

#include <lib/math.h>

int pow(int base, int exponent)
{
    double res = 1;
    for (int i = 0; i < exponent; i++)
        res = res * base;
    return res;
}

float sqrt(float x)
{
    float result = 0;
    __asm__ volatile(
        "flds %1\n"
        "fsqrt\n"
        "fstps %0\n"
        : "=m"(result)
        : "m"(x));
    return result;
}

uint16_t reverse_endian(uint16_t nb)
{
    return (nb >> 8) | (nb << 8);
}
