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
bits 64

global syscall_entry
global ret_from_fork
global ret_from_fork_syscall
extern syscall_handler

syscall_entry:
    swapgs
    mov [gs:0x08], rsp
    mov rsp, [gs:0x00]
    push qword [gs:0x08]

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
    swapgs
    o64 sysret

ret_from_fork:
    jmp ret_from_fork_common

ret_from_fork_syscall:
    swapgs

ret_from_fork_common:
    movzx eax, word [rsp + 144]
    and al, 3
    cmp al, 3
    jne .kernel_segments
    mov ax, 0x3B
    jmp .load_segments
    .kernel_segments:
        mov ax, 0x30
    .load_segments:
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax

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
