#ifndef LOOP_H
#define LOOP_H

#include "base/base.h"
#include "platform/platform.h"

#define LOOP_WATCH_MAX 16
#define LOOP_TIMER_MAX 4

typedef struct LoopTimer LoopTimer;
struct LoopTimer 
{
    b32 is_running;
    b32 has_fired;
    u64 deadline_ms;
    u64 interval_ms;
};

typedef struct Loop Loop;
struct Loop
{
    PlatformWatch watches[LOOP_WATCH_MAX];
    u32 watch_count;
    LoopTimer timers[LOOP_TIMER_MAX];
    u32 timer_count;
    u64 now_ms;
    int stop_signal;
};

u64 loop_now_ms(void);
Loop loop_create(void);

void loop_watch(Loop *l, PlatformHandle h);
void loop_watch_write(Loop *l, PlatformHandle h, b32 should_write);
void loop_unwatch(Loop *l, PlatformHandle h);

u32 loop_timer_add(Loop *l);
void loop_timer_repeat(Loop *l, u32 timer, u64 interval_ms);
void loop_timer_start(Loop *l, u32 timer, u64 delay_ms);
void loop_timer_stop(Loop *l, u32 timer);

b32 loop_wait(Loop *l);
b32 loop_is_readable(const Loop *l, PlatformHandle h);
b32 loop_is_writable(const Loop *l, PlatformHandle h);
b32 loop_has_timer_fired(const Loop *l, u32 timer);

int loop_timeout_ms(const Loop *l);
void loop_fire_timers(Loop *l);

#endif