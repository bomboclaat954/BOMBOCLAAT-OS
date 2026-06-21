/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RAND_H
#define RAND_H
#include <stdint.h>

extern uint32_t rand_state;

#ifdef __cplusplus
extern "C"
{
#endif

    uint32_t rand();
    void srand(uint32_t seed);
    void random_seed();

#ifdef __cplusplus
}
#endif

#endif
