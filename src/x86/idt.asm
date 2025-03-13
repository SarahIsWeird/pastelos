IDT_PRESENT     equ 0x80
IDT_DPL_0       equ 0x00
IDT_DPL_3       equ 0x60
IDT_INT_GATE    equ 0x0e
IDT_TRAP_GATE   equ 0x0f

%macro idt_entry 2
idt_entry_%1:
    dw 0                ; loword of the isr addr
    dw 0x08             ; the GDT entry to use
    db 0                ; reserved
    db IDT_PRESENT | %2 ; flags
    dw 0                ; hiword of the isr addr
%endmacro

%macro idt_int 1
    idt_entry %1, IDT_INT_GATE | IDT_DPL_0
%endmacro

%macro idt_trap 1
    idt_entry %1, IDT_TRAP_GATE | IDT_DPL_0
%endmacro

%macro idt_syscall 1
    idt_entry %1, IDT_INT_GATE | IDT_DPL_3
%endmacro

%macro isr 1
isr_%1:
    push 0
    push %1
    jmp common_isr
%endmacro

%macro isr_error 1
isr_%1:
    push %1
    jmp common_isr
%endmacro

%macro set_isr 1
    mov eax, isr_%1
    mov word [idt_entry_%1], ax
    shr eax, 16
    mov word [idt_entry_%1 + 6], ax
%endmacro

section .data
idtr:
    dw 0 ; size
    dd 0 ; offset

idt:
    idt_int 0 ;#DE
    idt_int 1
    idt_int 2
    idt_trap 3
    idt_trap 4
    idt_int 5
    idt_int 6
    idt_int 7
    idt_int 8
    idt_int 9
    idt_int 10
    idt_int 11
    idt_int 12
    idt_int 13
    idt_int 14
    idt_int 15
    idt_int 16
    idt_int 17
    idt_int 18
    idt_int 19
    idt_int 20
    idt_int 21
    idt_int 22
    idt_int 23
    idt_int 24
    idt_int 25
    idt_int 26
    idt_int 27
    idt_int 28
    idt_int 29
    idt_int 30
    idt_int 31
%assign i 32
%rep 256 - 32
%if i == 0x69
    idt_syscall i
%else
    idt_int i
%endif
%assign i i+1
%endrep
IDT_SIZE equ $ - idt

section .text

isrs:
    isr 0
    isr 1
    isr 2
    isr 3
    isr 4
    isr 5
    isr 6
    isr 7
    isr_error 8
    isr 9
    isr_error 10
    isr_error 11
    isr_error 12
    isr_error 13
    isr_error 14
    isr 15
    isr 16
    isr_error 17
    isr 18
    isr 19
    isr 20
    isr_error 21
    isr 22
    isr 23
    isr 24
    isr 25
    isr 26
    isr 27
    isr 28
    isr_error 29
    isr_error 30
    isr 31
%assign i 32
%rep 256 - 32
    isr i
%assign i i+1
%endrep

extern handle_interrupt

common_isr:
    pushad
    push esp

    mov ax, 0x10
    mov ds, ax
    mov es, ax

    cld
    call handle_interrupt
    mov esp, eax

    mov ax, 0x23
    mov ds, ax
    mov es, ax

    popad
    add esp, 8
    iret

global setup_idt
setup_idt:
%assign i 0
%rep 256
    set_isr i
%assign i i+1
%endrep
    mov word [idtr], IDT_SIZE - 1
    mov dword [idtr + 2], idt
    lidt [idtr]
    ret

global enable_interrupts
enable_interrupts:
    sti
    ret
