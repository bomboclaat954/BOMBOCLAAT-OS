; * BOMBOCLAAT-OS - simple x86_64 operating system
; * Copyright (C) 2026 Jakub Fietko <fietkojakub@proton.me>
; *
; * This program is free software: you can redistribute it and/or modify
; * it under the terms of the GNU General Public License as published by
; * the Free Software Foundation, either version 3 of the License, or
; * (at your option) any later version.
; *
; * This program is distributed in the hope that it will be useful,
; * but WITHOUT ANY WARRANTY; without even the implied warranty of
; * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
; * GNU General Public License for more details.
; *
; * You should have received a copy of the GNU General Public License
; * along with this program. If not, see <https://www.gnu.org/licenses/>.
; Fuck assembly
; jakim kurwa cwelem jebanym trzeba być żeby takie chujostwo wymyślić to ja pierdole
; co za jebany kurwa w dupe pojeb to stworzył, serio kurwa niech go walec rozjedzie
bits 64
default rel

global syscall_entry
global ret_from_fork
global ret_from_fork_syscall
extern syscall_handler
extern syscall_kernel_rsp

section .bss
align 8
syscall_user_rsp: resq 1

section .text

syscall_entry:
    mov [syscall_user_rsp], rsp
    mov rsp, [syscall_kernel_rsp]
    push qword [syscall_user_rsp]

    push r15
    push r14
    push r13
    push r12
    push rbp
    push rbx
    push r11
    push rcx
    push r9
    push r8
    push r10
    push rdx
    push rsi
    push rdi
    push rax

    cld
    mov rdi, rsp
    call syscall_handler

    add rsp, 8
    pop rdi
    pop rsi
    pop rdx
    pop r10
    pop r8
    pop r9
    pop rcx
    pop r11
    pop rbx
    pop rbp
    pop r12
    pop r13
    pop r14
    pop r15

    pop rsp
    o64 sysret

ret_from_fork:
ret_from_fork_syscall:
    test byte [rsp + 144], 3
    mov ax, 0x30
    jz .load_segments
    mov ax, 0x3B
.load_segments:
    mov ds, ax
    mov es, ax

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16

    iretq
