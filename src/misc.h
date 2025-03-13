#ifndef MISC_H
#define MISC_H

#include <stdint.h>

// If this is included after stddef.h, it might be already defined.
#ifndef NULL
# define NULL ((void *) 0)
#endif

void outb(uint16_t port, uint8_t byte);

void memset(void *ptr, uint8_t byte, uint32_t count);
void memcpy(void *restrict dst, const void *restrict src, uint32_t count);

void panic(const char *fmt, ...);

#endif
