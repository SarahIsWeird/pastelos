#ifndef SYSCALL_H
#define SYSCALL_H

#include "../x86/idt.h"

#define SYSCALL_EXIT        0x00
#define SYSCALL_WRITE       0x01

int_ctx_t *syscall_handle(int_ctx_t *ctx);

#endif
