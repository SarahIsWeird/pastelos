section .data
gdtr:
    dw 0 ; limit
    dd 0 ; base
    dw 0

section .text

global set_gdtr
set_gdtr:
    mov eax, dword [esp + 4]
    mov dword [gdtr + 2], eax
    mov ax, word [esp + 8]
    mov word [gdtr], ax
    lgdt [gdtr]
    jmp 0x08:.reload_cs
.reload_cs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

global set_tr
set_tr:
    mov eax, dword [esp + 4]
    mov dword [eax + 0x4], esp     ; kernel stack
    mov dword [eax + 0x8], 0x10    ; kernel ss
    mov eax, dword [esp + 8]
    ltr ax
    ret
