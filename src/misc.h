#ifndef MISC_H
#define MISC_H

#include <stdint.h>

void outb(uint16_t port, uint8_t byte);

void memset(void *ptr, uint8_t byte, uint32_t count);
void memcpy(void *restrict dst, const void *restrict src, uint32_t count);

void panic(const char *fmt, ...);

#endif
