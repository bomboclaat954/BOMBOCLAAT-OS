/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SYSCALL_H
#define SYSCALL_H

typedef struct
{
    uint64_t sys_num; // RAX
    uint64_t arg1;    // RDI
    uint64_t arg2;    // RSI
    uint64_t arg3;    // RDX
    uint64_t arg4;    // R10
    uint64_t arg5;    // R8
    uint64_t arg6;    // R9
    uint64_t rip;     // RCX
    uint64_t rflags;  // R11
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
} __attribute__((packed)) syscall_ctx_t;

void init_syscall(uint16_t kernel_cs, uint16_t user_cs_base);

#endif
