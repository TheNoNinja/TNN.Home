#include "loop/loop.h"
#include "base/base_assert.h"
#include "base/base_log.h"

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <time.h>

internal volatile sig_atomic_t loop_signal_received;

internal void loop_signal_handler(const int number)
{
    loop_signal_received = number;
}

internal b32 loop_take_signal(Loop *l)
{
    if (!loop_signal_received)
    {
        return 0;
    }
    l->stop_signal = (int)loop_signal_received;
    loop_signal_received = 0;
    return 1;
}

u64 loop_now_ms(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

Loop loop_create(void)
{
    Loop l = {0};
    l.now_ms = loop_now_ms();
    return l;
}

void loop_catch_signals(void)
{
    struct sigaction action = {0};
    action.sa_handler = loop_signal_handler;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, 0);
    sigaction(SIGTERM, &action, 0);
}

void loop_watch(Loop *l, const int fd)
{
    ASSERT(l->fd_count < LOOP_FDS_MAX);
    l->fds[l->fd_count].fd = fd;
    l->fds[l->fd_count].events = POLLIN;
    l->fd_count += 1;
}

u32 loop_timer_add(Loop *l)
{
    ASSERT(l->timer_count < LOOP_TIMERS_MAX);
    l->timer_count += 1;
    return l->timer_count - 1;
}

void loop_timer_start(Loop *l, const u32 timer, const u64 delay_ms)
{
    ASSERT(timer < l->timer_count && delay_ms <= INT_MAX);
    l->timers[timer].running = 1;
    l->timers[timer].fired = 0;
    l->timers[timer].deadline_ms = l->now_ms + delay_ms;
}

void loop_timer_stop(Loop *l, const u32 timer)
{
    ASSERT(timer < l->timer_count);
    l->timers[timer].running = 0;
    l->timers[timer].fired = 0;
}

int loop_timeout_ms(const Loop *l)
{
    i64 timeout = -1;
    for (u32 i = 0; i < l->timer_count; i++)
    {
        const LoopTimer *t = &l->timers[i];
        if (!t->running)
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
        t->fired = t->running && t->deadline_ms <= l->now_ms;
        if (t->fired)
        {
            t->running = 0;
        }
    }
}

b32 loop_wait(Loop *l)
{
    if (loop_take_signal(l))
    {
        return 0;
    }

    for(u32 i = 0; i < l->fd_count; i++)
    {
        l->fds[i].revents = 0;
    }
    l->now_ms = loop_now_ms();
    const int ready = poll(l->fds, l->fd_count, loop_timeout_ms(l));

    if(loop_take_signal(l))
    {
        return 0;
    }
    if (ready < 0 && errno != EINTR)
    {
        LOG_ERROR("poll: %s", strerror(errno));
        return 0;
    }

    l->now_ms = loop_now_ms();
    loop_fire_timers(l);
    return 1;
}

b32 loop_readable(const Loop *l, const int fd)
{
    for (u32 i = 0; i < l->fd_count; i++)
    {
        if (l->fds[i].fd == fd)
        {
            return l->fds[i].revents != 0;
        }
    }
    return 0;
}

b32 loop_timer_fired(const Loop *l, const u32 timer)
{
    ASSERT(timer < l->timer_count);
    return l->timers[timer].fired;
}