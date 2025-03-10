section .text

global invalidate_page
invalidate_page:
    invlpg [esp + 4]
    ret

global enable_paging
enable_paging:
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret

global set_active_page_directory
set_active_page_directory:
    mov eax, dword [esp + 4]
    mov cr3, eax
    ret

global flush_pd
flush_pd:
    mov eax, cr0
    mov cr0, eax
    ret
