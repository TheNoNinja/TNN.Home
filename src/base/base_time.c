#include "base/base_time.h"

#include <time.h>

internal u64 time_read_ns(const clockid_t clock)
{
    struct timespec now;
    clock_gettime(clock, &now);
    return (u64)now.tv_sec * TIME_NS_PER_S + (u64)now.tv_nsec;
}

u64 time_now_raw_ns(void)
{
    return time_read_ns(CLOCK_MONOTONIC_RAW);
}

u64 time_now_mono_ns(void)
{
    return time_read_ns(CLOCK_MONOTONIC);
}