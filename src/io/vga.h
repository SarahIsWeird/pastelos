#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stdarg.h>

void vga_putc(char c);
void vga_print(const char *str);

void vga_printf(const char *fmt, ...);
// It is the responsibility of the caller to call both va_start and va_end.
void vga_vprintf(const char *fmt, va_list args); 

void vga_set_color(uint8_t color);
void vga_clear();

#endif
