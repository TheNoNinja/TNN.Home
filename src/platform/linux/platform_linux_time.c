#define _POSIX_C_SOURCE 200809L
#include "platform/platform_time.h"
#include "platform/linux/platform_linux.h"
#include "base/base_time.h"

#include <time.h>

internal u64 platform_time_read_ns(const clockid_t clock)
{
    struct timespec now;
    clock_gettime(clock, &now);
    return (u64)now.tv_sec * TIME_NS_PER_S + (u64)now.tv_nsec;
}

u64 platform_time_raw_ns(void)
{
    return platform_time_read_ns(CLOCK_MONOTONIC_RAW);
}

u64 platform_time_mono_ns(void)
{
    return platform_time_read_ns(CLOCK_MONOTONIC);
}