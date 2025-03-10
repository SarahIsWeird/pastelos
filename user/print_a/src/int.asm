section .text
global putc
putc:
    push ebp
    mov ebp, esp

    mov ebx, dword [esp + 8]
    mov eax, 0
    int 0x69

    leave
    ret
