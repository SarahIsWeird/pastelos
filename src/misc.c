#include "misc.h"

#include <stdarg.h>

#include "io/vga.h"

void memset(void *ptr, uint8_t byte, uint32_t count) {
    uint8_t *dst = (uint8_t *) ptr;

    for (uint32_t i = 0; i < count; i++) {
        dst[i] = byte;
    }
}

void memcpy(void *restrict dst, const void *restrict src, uint32_t count) {
    uint8_t *dst8 = (uint8_t *) dst;
    uint8_t *src8 = (uint8_t *) src;

    for (uint32_t i = 0; i < count; i++) {
        dst8[i] = src8[i];
    }
}

void panic(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vga_set_color(0x0c);
    vga_vprintf(fmt, args);

    va_end(args);

    while (1) {}

    __builtin_unreachable();
}
