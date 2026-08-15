#include "base/base_time.h"

#include <time.h>

u64 time_now_raw_ns(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    return (u64)now.tv_sec * TIME_NS_PER_S + (u64)now.tv_nsec;
}

