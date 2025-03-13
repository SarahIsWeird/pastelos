MB_ALIGN equ 0x01
MB_MEMINFO equ 0x02
MB_FLAGS equ MB_ALIGN | MB_MEMINFO
MB_MAGIC equ 0x1badb002
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

VIRT_OFFSET equ 0xc0000000

section .multiboot
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

section .bss
stack_bottom:   resb 16384
stack_top:

global init_pd
align 4096
init_pd:        resb 4096

global init_pt
align 4096
init_pt:        resb 4096

; pain and suffering.
global mb_info_pt
align 4096
mb_info_pt:     resb 4096


section .data
mb_info:        dd 0
mb_checksum:    dd 0
global kernel_start
kernel_start:   dd 0
global kernel_end
kernel_end:     dd 0

section .entry
extern _kernel_start
extern _kernel_end
extern mb_start
extern rw_start

global _start
_start:
    ; this sets up an extremely minimalistic page directory.
    ; there's no stack yet, and we need to take care to save
    ; the multiboot info!
    mov dword [mb_checksum - VIRT_OFFSET], eax
    mov dword [mb_info - VIRT_OFFSET], ebx

    mov eax, init_pt - VIRT_OFFSET
    mov ebx, 0

.map_high:
    mov edx, ebx
    or edx, 3 ; P_PRESENT
;     cmp ebx, rw_start - VIRT_OFFSET
;     jl .not_writable
;     or edx, 2 ; P_WRITABLE
; .not_writable:
    mov dword [eax], edx
    add eax, 4
    add ebx, 4096
    cmp ebx, _kernel_end - VIRT_OFFSET
    jl .map_high

    ; map the vga mem onto itself. luckily, it fits into one page :)
    ; and since we're loaded at 1M, it doesn't clash with the kernel mapping either.
    mov dword [init_pt - VIRT_OFFSET + 0xb8], 0xb8000 | 0x3

    ; identity map the kernel
    mov eax, init_pt - VIRT_OFFSET  ; can't do `|` on addresses
    or eax, 3                       ; P_PRESENT | P_WRITABLE
    mov dword [init_pd - VIRT_OFFSET], eax

    ; and add the higher half mapping
    VIRT_PD_OFFSET equ 0xc0000000 / 4096 / 1024 * 4
    mov dword [init_pd - VIRT_OFFSET + 768 * 4], eax

    ; add pointer to itself at the last pd entry
    mov eax, init_pd - VIRT_OFFSET
    or eax, 3
    mov dword [init_pd - VIRT_OFFSET + 4092], eax

    ; map multiboot struct
    mov ecx, dword [mb_info - VIRT_OFFSET]
    mov eax, ecx
    mov ebx, ecx
    shr ecx, 22
    jz .same_pt_as_the_rest

    ; pt index
    shr eax, 12
    and eax, 0x3ff
    ; pt entry
    and ebx, ~0xfff
    or ebx, 3
    mov dword [mb_info_pt - VIRT_OFFSET + eax * 4], ebx

    ; put mb mapping into pd
    mov dword [init_pd - VIRT_OFFSET + ecx * 4], (mb_info_pt - VIRT_OFFSET + 3)
    jmp .virt_setup_done

.same_pt_as_the_rest:
    ; pt index
    shr eax, 12
    and eax, 0x3ff
    ; pt entry
    and ebx, ~0xfff
    or ebx, 3
    mov dword [init_pt - VIRT_OFFSET + eax * 4], ebx

.virt_setup_done:
    ; set the pd and enable paging
    mov eax, init_pd - VIRT_OFFSET
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80010000 ; PG = 1, WP = 1
    mov cr0, eax

    mov dword [kernel_start], _kernel_start + VIRT_OFFSET
    mov dword [kernel_end], _kernel_end + VIRT_OFFSET

    mov eax, dword [mb_checksum - VIRT_OFFSET]
    mov ebx, dword [mb_info - VIRT_OFFSET]
    add ebx, VIRT_OFFSET

    jmp real_start

section .text
global real_start
real_start:
    mov esp, stack_top
    push eax
    push ebx
extern main
    call main
    cli
.hang:
    hlt
    jmp .hang

; misc.h
global outb
outb:
    mov al, byte [esp + 8]
    mov dx, word [esp + 4]
    out dx, al
    ret
