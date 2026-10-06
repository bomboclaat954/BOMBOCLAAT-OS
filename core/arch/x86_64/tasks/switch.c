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

#include <stddef.h>
#include <tasks/tasks.h>

_Static_assert(sizeof(registers_t) == 176, "registers_t layout is assumed by the assembly code");

__asm__(
    ".text\n"
    ".globl cpu_switch_context\n"
    ".type cpu_switch_context, @function\n"
    "cpu_switch_context:\n"
    "    pushq %rbp\n"
    "    pushq %rbx\n"
    "    pushq %r12\n"
    "    pushq %r13\n"
    "    pushq %r14\n"
    "    pushq %r15\n"
    "    movq %rsp, (%rdi)\n"
    "    movq %rsi, %rsp\n"
    "    popq %r15\n"
    "    popq %r14\n"
    "    popq %r13\n"
    "    popq %r12\n"
    "    popq %rbx\n"
    "    popq %rbp\n"
    "    ret\n"
    ".size cpu_switch_context, .-cpu_switch_context\n"
    "\n"
    ".globl enter_user_context\n"
    ".type enter_user_context, @function\n"
    "enter_user_context:\n"
    "    cld\n"
    "    leaq -176(%rsi), %rax\n"
    "    movq %rdi, %rsi\n"
    "    movq %rax, %rdi\n"
    "    movq %rax, %rsp\n"
    "    movl $22, %ecx\n"
    "    rep movsq\n"
    "    jmp ret_from_fork\n"
    ".size enter_user_context, .-enter_user_context\n");
