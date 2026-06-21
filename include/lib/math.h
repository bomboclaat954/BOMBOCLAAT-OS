/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MATH_H
#define MATH_H
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int pow(int base, int exponent);
    float sqrt(float x);
    uint16_t reverse_endian(uint16_t nb);

#define CEILING_POS(X) ((X - (int)(X)) > 0 ? (int)(X + 1) : (int)(X))
#define CEILING_NEG(X) (int)(X)
#define CEILING(X) (((X) > 0) ? CEILING_POS(X) : CEILING_NEG(X))

#ifdef __cplusplus
}
#endif

#endif
