;BOMBOCLAAT-OS boot.asm 
;This is the entry point of the system. It tells GRUB that there's a kernel to load and jumps into it.
;Some of this code is probably useless but it works so don't touch it

[BITS 32]

SECTION .text
ALIGN 4
    MB_MAGIC    equ 0x1BADB002
    MB_FLAGS    equ 0x00000003
    MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

mboot:
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM
    EXTERN code, bss, end

GLOBAL start
start:
    cli
    mov edi, eax
    mov esi, ebx

    lgdt [gdt_descr]
    jmp 0x08:.reload_cs
.reload_cs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, kstack + 4096

    push esi        ; mboot_info
    push edi        ; magic
    EXTERN start_kernel
    call start_kernel ;jump into kernel_main function passing magic and mboot_info
    cli
.hang:
    hlt
    jmp .hang

SECTION .bss
ALIGN 16
resb 1024
kstack: 
    resb 16384
kstack_end:

SECTION .data
ALIGN 4
GLOBAL stack_guard
stack_guard:
    dd 0xDEADBEEF
gdt:
    dd 0, 0 
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00

gdt_descr:
    dw $ - gdt - 1
    dd gdt
