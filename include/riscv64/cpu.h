/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef RISCV64_CPU_H
#define RISCV64_CPU_H
#include <stdint.h>

/*
 * RISC-V 64 registers explained:
 * x0                   - always 0, writing to it is ignored
 * x1: ra               - return address
 * x2: sp               - stack pointer
 * x3: gp               - global pointer
 * x4: tp               - thread pointer
 * x5 - x7: t0 - t2     - temporary registers
 * x8: s0/fp            - saved register/frame pointer
 * x9: s1               - saved register
 * x10 - x17: a0 - a7   - function arguments (a0 and a1 are also return values)
 * x18 - x27: s2 - s11  - saved registers
 * x28 - x31: t3 - t6   - temporary registers
 */
typedef struct __attribute__((packed))
{
    uint64_t zero;     // x0
    uint64_t ra;       // x1
    uint64_t sp;       // x2
    uint64_t gp;       // x3
    uint64_t tp;       // x4
    uint64_t t0_t2[3]; // x5 - x7 (t0, t1, t2)
    uint64_t s0;       // x8 / fp
    uint64_t s1;       // x9
    uint64_t a[8];     // x10 - x17 (a0 - a7)
    uint64_t s[10];    // x18 - x27 (s2 - s11)
    uint64_t t3_t6[4]; // x28 - x31 (t3 - t6)
} registers_t;

#endif
