section .text
global putc
putc:
    push ebp
    mov ebp, esp
    push ebx

    mov ebx, dword [ebp + 8]
    mov eax, 1
    int 0x69

    pop ebx
    leave
    ret

global exit
exit:
    xor eax, eax
    int 0x69

.scream:
    mov ebx, 0xc000 | 'A'
    inc eax
    int 0x60

    jmp .scream

