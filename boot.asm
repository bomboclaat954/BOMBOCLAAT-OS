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
    mov edi, eax    ; zachowaj magic w EDI (GRUB go nie używa)
    mov esi, ebx    ; zachowaj mboot_info w ESI

    lgdt [gdt_descr]
    jmp 0x08:.reload_cs
.reload_cs:
    mov ax, 0x10    ; to nadpisuje EAX!
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, kstack + 4096

    push esi        ; 2. argument: mboot_info (oryginalne EBX)
    push edi        ; 1. argument: magic (oryginalne EAX)
    EXTERN start_kernel
    call start_kernel
    cli
.hang:
    hlt
    jmp .hang

SECTION .bss
ALIGN 16
kstack: 
    resb 4096

SECTION .data
ALIGN 4
gdt:
    dd 0, 0 
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00

gdt_descr:
    dw $ - gdt - 1
    dd gdt
