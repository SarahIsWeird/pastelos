#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define TIMER_PIT 0x00

#define MAX_TIMER_COUNT 32

// us_between is on a best-effort basis. if it isn't feasible, like for the PIT, it may be slower/faster.
void timer_init(int timer_type, uint32_t us_between);
int timer_get_type();

void timer_tick();

// might be shorter or longer, the PIT is very bad!
int timer_new_oneshot(uint32_t ms);
int timer_oneshot_is_done(int timer_id);
// Convenience method that combines the above.`
void timer_sleep(uint32_t ms);

#endif
