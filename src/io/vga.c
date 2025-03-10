#include "vga.h"

#include <stdarg.h>
#include <stdint.h>

#define WIDTH 80
#define HEIGHT 25

static volatile uint16_t *text_buf = (uint16_t *) 0xc00b8000;

static volatile unsigned int line = 0;
static volatile unsigned int column = 0;
static volatile uint16_t color = 0xf000;

static void advance_line() {
    column = 0;
    line++;

    if (line == HEIGHT) {
        line--;

        for (int i = 0; i < WIDTH * (HEIGHT - 1); i++) {
            text_buf[i] = text_buf[i + WIDTH];
        }

        for (int i = WIDTH * (HEIGHT - 1); i < WIDTH * HEIGHT; i++) {
            text_buf[i] = color | ' ';
        }
    }
}

static void advance() {
    column++;
    if (column == WIDTH) {
        advance_line();
    }
}

const char *DIGITS = "0123456789abcdef";
static void putn(uint32_t n, int base, int padding) {
    char buf[32];
    buf[31] = 0;
    int i = 30;

    do {
        int digit = n % base;
        buf[i--] = DIGITS[digit];
        n /= base;
    } while (n > 0);

    int n_len = 30 - i;
    for (int j = n_len; j < padding; j++) {
        vga_putc('0');
    }

    vga_print(buf + i + 1);
}

static void putsn(int32_t n, int base, int padding) {
    if (n < 0) {
        n = -n;
        vga_putc('-');
    }

    putn(n, base, padding);
}

void vga_putc(char c) {
    if (c == '\n') {
        advance_line();
        return;
    }

    text_buf[column + line * WIDTH] = color | c;
    advance();
}

void vga_print(const char *str) {
    while (*str) {
        vga_putc(*(str++));
    }
}

void vga_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vga_vprintf(fmt, args);

    va_end(args);
}

void vga_vprintf(const char *fmt, va_list args) {
    for (int i = 0; fmt[i] != 0; i++) {
        char c = fmt[i];
        if (c != '%') {
            vga_putc(c);
            continue;
        }

        c = fmt[++i];
        int padding = 0;
        while (c >= '0' && c <= '9') {
            int padding_digit = c - '0';
            padding = (padding << 10) + padding_digit;
            c = fmt[++i];
        }

        switch (c) {
        case '%': vga_putc('%'); continue;
        case 's': {
            const char *str = va_arg(args, const char *);
            vga_print(str);
            continue;
        }
        case 'd': {
            int32_t n = va_arg(args, int32_t);
            putsn(n, 10, padding);
            continue;
        }
        case 'u': {
            uint32_t n = va_arg(args, uint32_t);
            putn(n, 10, padding);
            continue;
        }
        case 'x':
        case 'X': {
            uint32_t n = va_arg(args, uint32_t);
            putn(n, 16, padding);
            continue;
        }
        case 'p': {
            uint32_t n = va_arg(args, uint32_t);
            putn(n, 16, 8);
            continue;
        }
        default:
            vga_putc('%');
            vga_putc(c);
            continue;
        }
    }
}

void vga_set_color(uint8_t new_color) {
    color = ((uint16_t) new_color) << 8;
}

void vga_clear() {
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        text_buf[i] = color | ' ';
    }

    column = 0;
    line = 0;
}
