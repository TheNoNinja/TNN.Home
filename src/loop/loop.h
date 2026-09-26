#ifndef LOOP_H
#define LOOP_H

#include "base/base.h"

#include <poll.h>

#define LOOP_FDS_MAX 16
#define LOOP_TIMERS_MAX 4

typedef struct LoopTimer LoopTimer;
struct LoopTimer 
{
    b32 is_running;
    b32 has_fired;
    u64 deadline_ms;
};

typedef struct Loop Loop;
struct Loop
{
    struct pollfd fds[LOOP_FDS_MAX];
    u32 fd_count;
    LoopTimer timers[LOOP_TIMERS_MAX];
    u32 timer_count;
    u64 now_ms;
    int stop_signal;
};

u64 loop_now_ms(void);
Loop loop_create(void);
void loop_catch_signals(void);

void loop_watch(Loop *l, int fd);
void loop_unwatch(Loop *l, int fd);

u32 loop_timer_add(Loop *l);
void loop_timer_start(Loop *l, u32 timer, u64 delay_ms);
void loop_timer_stop(Loop *l, u32 timer);

b32 loop_wait(Loop *l);
b32 loop_readable(const Loop *l, int fd);
b32 loop_timer_fired(const Loop *l, u32 timer);

int loop_timeout_ms(const Loop *l);
void loop_fire_timers(Loop *l);

#endif