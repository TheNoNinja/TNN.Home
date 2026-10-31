#include "loop/loop.h"

#include <limits.h>

internal b32 loop_take_signal(Loop *l)
{
    const int number = platform_signal_take();
    if (number == 0)
    {
        return 0;
    }
    l->stop_signal = number;
    return 1;
}

u64 loop_now_ms(void)
{
    return platform_time_mono_ns() / TIME_NS_PER_MS;
}

Loop loop_create(void)
{
    Loop l = {0};
    l.now_ms = loop_now_ms();
    return l;
}

internal u32 loop_find(const Loop *l, const PlatformHandle h)
{
    u32 i = 0;
    while (i < l->watch_count && l->watches[i].handle.value != h.value)
    {
        i++;
    }
    return i;
}

void loop_watch(Loop *l, const PlatformHandle h)
{
    ASSERT(l->watch_count < LOOP_WATCH_MAX);
    l->watches[l->watch_count] = (PlatformWatch){ .handle = h };
    l->watch_count += 1;
}

void loop_watch_write(Loop *l, const PlatformHandle h, const b32 should_write)
{
    const u32 i = loop_find(l, h);
    if (i < l->watch_count)
    {
        l->watches[i].should_write = should_write;
    }
}

void loop_unwatch(Loop *l, const PlatformHandle h)
{
    const u32 i = loop_find(l, h);
    if (i < l->watch_count)
    {
        l->watches[i] = l->watches[l->watch_count - 1];
        l->watch_count -= 1;
    }
}

u32 loop_timer_add(Loop *l)
{
    ASSERT(l->timer_count < LOOP_TIMER_MAX);
    l->timer_count += 1;
    return l->timer_count - 1;
}

void loop_timer_repeat(Loop *l, const u32 timer, const u64 interval_ms)
{
    ASSERT(interval_ms > 0);
    loop_timer_start(l, timer, interval_ms);
    l->timers[timer].interval_ms = interval_ms;
}

void loop_timer_start(Loop *l, const u32 timer, const u64 delay_ms)
{
    ASSERT(timer < l->timer_count && delay_ms <= INT_MAX);
    l->timers[timer].is_running = 1;
    l->timers[timer].has_fired = 0;
    l->timers[timer].deadline_ms = l->now_ms + delay_ms;
    l->timers[timer].interval_ms = 0;
}

void loop_timer_stop(Loop *l, const u32 timer)
{
    ASSERT(timer < l->timer_count);
    l->timers[timer].is_running = 0;
    l->timers[timer].has_fired = 0;
}

int loop_timeout_ms(const Loop *l)
{
    i64 timeout = -1;
    for (u32 i = 0; i < l->timer_count; i++)
    {
        const LoopTimer *t = &l->timers[i];
        if (!t->is_running)
        {
            continue;
        }

        const i64 left = t->deadline_ms > l->now_ms ? (i64)(t->deadline_ms - l->now_ms) : 0;
        if (timeout < 0 || left < timeout)
        {
            timeout = left;
        }
    }
    return (int)timeout;
}

void loop_fire_timers(Loop *l)
{
    for (u32 i = 0; i < l->timer_count; i++)
    {
        LoopTimer *t = &l->timers[i];
        t->has_fired = t->is_running && t->deadline_ms <= l->now_ms;
        if (!t->has_fired)
        {
            continue;
        }
        if (t->interval_ms == 0)
        {
            t->is_running = 0;
            continue;
        }

        t->deadline_ms += t->interval_ms;
        if(t->deadline_ms <= l->now_ms)
        {
            t->deadline_ms = l->now_ms + t->interval_ms;
        }
    }
}

b32 loop_wait(Loop *l)
{
    if (loop_take_signal(l))
    {
        return 0;
    }

    l->now_ms = loop_now_ms();
    const b32 is_awake = platform_wait(l->watches, l->watch_count, loop_timeout_ms(l));

    if(loop_take_signal(l))
    {
        return 0;
    }
    if (!is_awake)
    {
        LOG_ERROR("wait: %s", platform_error());
        return 0;
    }

    l->now_ms = loop_now_ms();
    loop_fire_timers(l);
    return 1;
}

b32 loop_is_readable(const Loop *l, const PlatformHandle h)
{
    const u32 i = loop_find(l, h);
    return i < l->watch_count && l->watches[i].is_readable;
}

b32 loop_is_writable(const Loop *l, const PlatformHandle h)
{
    const u32 i = loop_find(l, h);
    return i < l->watch_count && l->watches[i].is_writable;
}

b32 loop_has_timer_fired(const Loop *l, const u32 timer)
{
    ASSERT(timer < l->timer_count);
    return l->timers[timer].has_fired;
}