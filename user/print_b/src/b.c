#include <stdint.h>

extern void putc(uint32_t c);

void _start() {
    for (int i = 0; i < 4; i++) {
        putc(0x0f00 | 'b');
        // vga[i] = 0x0f00 | 'A';
    }

    while (1);
}
