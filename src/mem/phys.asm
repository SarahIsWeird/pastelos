section .data
lck:
    dd 0

section .text
global phys_acquire_lock
phys_acquire_lock:
.spin:
    mov eax, dword [lck]
    test eax, eax
    jnz .spin
    mov dword [lck], 1
    ret

global phys_release_lock
phys_release_lock:
    mov dword [lck], 0
    ret
