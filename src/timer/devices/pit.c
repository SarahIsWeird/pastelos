#include "pit.h"

#include "../../misc.h"
#include "../../io/vga.h"

#define PIT_DATA_CH0 0x40
#define PIT_DATA_CH1 0x41
#define PIT_DATA_CH2 0x42
#define PIT_CMD      0x43

#define PIT_CH0         (0 << 6)
#define PIT_CH1         (1 << 6)
#define PIT_CH2         (2 << 6)
#define PIT_READ        (3 << 6)
#define PIT_ACC_LATCH   (0 << 4)
#define PIT_ACC_LO      (1 << 4)
#define PIT_ACC_HI      (2 << 4)
#define PIT_ACC_LOHI    (3 << 4)
#define PIT_INT_ON_TC   (0 << 1)
#define PIT_HW_ONESHOT  (1 << 1)
#define PIT_RATE_GEN    (2 << 1)
#define PIT_SQUARE_WAVE (3 << 1)
#define PIT_SW_STROBE   (4 << 1)
#define PIT_HW_STROBE   (5 << 1)
#define PIT_BINARY      0
#define PIT_BCD         1

#define PIT_FREQ        1193181 // in Hz

uint64_t pit_init(uint32_t) {
    // For some reason, QEMU really hates values below 8. :(
    // So we just try to aim for more or less 1ms instead of going for microseconds.
    // This is ~1.0007ms, so it's pretty close. We could do funny fractional stuff,
    // but honestly, it's good enough. The PIT isn't very stable anyways, at least
    // not in QEMU.
    uint16_t pit_count = 1194;

    outb(PIT_CMD, PIT_CH0 | PIT_ACC_LOHI | PIT_RATE_GEN | PIT_BINARY);
    outb(PIT_DATA_CH0, pit_count & 0xff);
    outb(PIT_DATA_CH0, (pit_count >> 8) & 0xff);

    return (uint64_t) pit_count * 1000000 / PIT_FREQ;
}
