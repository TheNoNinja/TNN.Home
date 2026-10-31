#include "base/base.h"
#include "histogram/histogram.h"
#include "platform/platform.h"

#include <poll.h>

#define TIMER_LATENESS_DELAY_MS 1
#define TIMER_LATENESS_ROUNDS   5000

int main(const int argc, char **argv)
{
    u64 rounds = TIMER_LATENESS_ROUNDS;
    if (argc > 1 && !cstr_to_u64_range(argv[1], 1, 1000000, &rounds))
    {
        LOG_ERROR("usage: %s [rounds(1 to 1000000, default %d)]", argv[0], TIMER_LATENESS_ROUNDS);
        return 1;
    }
    LOG_INFO("%llu rounds of platform_wait with a timeout of %d ms", (unsigned long long)rounds, TIMER_LATENESS_DELAY_MS);

    Histogram lateness = histogram_create();
    const u64 asked_ns = (u64)TIMER_LATENESS_DELAY_MS * TIME_NS_PER_MS;
    for (u64 i = 0; i < rounds; i++)
    {
        const u64 before = platform_time_raw_ns();
        platform_wait(0, 0, TIMER_LATENESS_DELAY_MS);
        const u64 slept_ns = platform_time_raw_ns() - before;
        histogram_record(&lateness, slept_ns > asked_ns ? slept_ns - asked_ns : 0);
    }

    histogram_print(&lateness, "timer lateness");
    return 0;
}
