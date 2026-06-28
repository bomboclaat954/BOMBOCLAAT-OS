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
extern main
global _start

_start:
    mov rdi, [rsp] ;argc
    lea rsi, [rsp + 8] ;argv
    call main
    
    mov rdi, rax    
    mov rax, 3
    int 0x80
    .dead_loop: ;in case if the syscall didn't work
        jmp .dead_loop
