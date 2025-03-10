#include "timer.h"

#include <stddef.h>

#include "../misc.h"
#include "../io/vga.h"
#include "devices/pit.h"

typedef struct timer_t {
    int id;
    uint64_t end;
} timer_t;

static int timer_type;
static uint64_t us_per_tick;

static volatile uint64_t timer_ticks = 0;
static int curr_timer_id = 1;
static timer_t timers[MAX_TIMER_COUNT];

static timer_t *find_timer(int id) {
    for (int i = 0; i < MAX_TIMER_COUNT; i++) {
        if (timers[i].id == id) return &timers[i];
    }

    return NULL;
}

void timer_init(int the_timer_type, uint32_t us_between) {
    timer_type = the_timer_type;
    memset(timers, 0, sizeof(timers));

    switch (timer_type) {
    case TIMER_PIT:
        us_per_tick = pit_init(us_between);
        break;
    default:
        panic("timer.c: Unknown timer type 0x%2x!\n", timer_type);
    }
}

int timer_get_type() {
    return timer_type;
}

void timer_tick() {
    timer_ticks++;
}

int timer_new_oneshot(uint32_t ms) {
    timer_t *free_slot = find_timer(0);
    if (free_slot == NULL) {
        panic("timer.c: Failed to find free timer slot!\n");
    }

    free_slot->id = curr_timer_id++;
    free_slot->end = timer_ticks + (uint64_t) ms * 1000 / us_per_tick;
    return free_slot->id;
}

int timer_oneshot_is_done(int timer_id) {
    timer_t *timer = find_timer(timer_id);
    if (timer == NULL) {
        panic("timer.c: No timer with id %d!\n", timer_id);
    }

    if (timer->end > timer_ticks) return 0;

    // Free up timer slot again
    timer->id = 0;
    timer->end = 0;
    return 1;
}

void timer_sleep(uint32_t ms) {
    int timer = timer_new_oneshot(ms);
    while (!timer_oneshot_is_done(timer));
}
