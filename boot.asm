[BITS 32]

SECTION .text
ALIGN 4
    MB_MAGIC    equ 0x1BADB002
    MB_FLAGS    equ 0x00010003
    MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

mboot:
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM
    EXTERN code, bss, end
    dd mboot
    dd code
    dd bss
    dd end
    dd start

GLOBAL start
start:
    push eax
    call start_kernel
    cli
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
    ;push ebx
    push eax

    EXTERN start_kernel
    call start_kernel
halt_loop:
    hlt
    jmp halt_loop

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
