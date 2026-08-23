#include "base/base.h"
#include "histogram/histogram.h"

#include <poll.h>
#include <stdlib.h>

#define TIMER_LATENESS_DELAY_MS 1
#define TIMER_LATENESS_ROUNDS   5000

int main(const int argc, char **argv)
{
    u32 rounds = TIMER_LATENESS_ROUNDS;
    if (argc > 1)
    {
        char *end = 0;
        const long value = strtol(argv[1], &end, 10);
        if (*end != '\0' || value < 1 || value > 1000000)
        {
            LOG_ERROR("usage: %s [rounds(1 to 1000000, default %d)]", argv[0], TIMER_LATENESS_ROUNDS);
            return 1;
        }
        rounds = (u32)value;
    }
    LOG_INFO("%u rounds of poll() with a timeout of %d ms", rounds, TIMER_LATENESS_DELAY_MS);

    Histogram lateness = histogram_create();
    const u64 asked_ns = (u64)TIMER_LATENESS_DELAY_MS * TIME_NS_PER_MS;
    for (u32 i = 0; i < rounds; i++)
    {
        const u64 before = time_now_raw_ns();
        poll(0, 0, TIMER_LATENESS_DELAY_MS);
        const u64 slept_ns = time_now_raw_ns() - before;
        histogram_record(&lateness, slept_ns > asked_ns ? slept_ns - asked_ns : 0);
    }

    histogram_print(&lateness, "timer lateness");
    return 0;
}
