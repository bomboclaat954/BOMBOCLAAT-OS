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

bits 64

global switch_to_task
switch_to_task:
    mov rax, cr3
    cmp rax, rsi
    je .skip_cr3
    mov cr3, rsi
    .skip_cr3:
        mov rsp, rdi

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
