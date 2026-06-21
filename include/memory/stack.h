/* BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STACK_H
#define STACK_H

#define MAX_SIZE 1024
#include <stdint.h>

typedef struct
{
    uintptr_t arr[MAX_SIZE];
    uintptr_t top;
} stack_t;

#ifdef __cplusplus
extern "C"
{

#endif
    void stack_init(stack_t *stack_t);
    int isEmpty(stack_t *stack_t);
    int isFull(stack_t *stack_t);
    void push(stack_t *stack_t, uintptr_t value);
    uintptr_t pop(stack_t *stack_t);
    uintptr_t top(stack_t *stack_t);

#ifdef __cplusplus
}
#endif

#endif