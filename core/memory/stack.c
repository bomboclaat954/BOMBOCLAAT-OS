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

#include <memory/stack.h>

void stack_init(stack_t *stack_t)
{
    stack_t->top = -1;
}

int isEmpty(stack_t *stack_t)
{
    return stack_t->top == -1;
}

int isFull(stack_t *stack_t)
{
    return stack_t->top >= MAX_SIZE - 1;
}

void push(stack_t *stack_t, uintptr_t value)
{
    if (isFull(stack_t))
        return;
    stack_t->arr[++stack_t->top] = value;
}

uintptr_t pop(stack_t *stack_t)
{
    if (isEmpty(stack_t))
        return -1;

    uintptr_t popped = stack_t->arr[stack_t->top];
    stack_t->top--;
    return popped;
}

uintptr_t top(stack_t *stack_t)
{
    if (isEmpty(stack_t))
        return -1;
    return stack_t->arr[stack_t->top];
}
