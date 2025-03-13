#include "syscall.h"

#include "../proc/proc.h"
#include "../io/vga.h"

int_ctx_t *syscall_handle(int_ctx_t *ctx) {
    switch (ctx->eax) {
    case SYSCALL_EXIT:
        proc_exit_current();
        return proc_get_current();
    case SYSCALL_WRITE:
        vga_set_color(ctx->ebx >> 8);
        vga_putc(ctx->ebx & 0xff);
        return ctx;
    default:
        return ctx;
    }
}
