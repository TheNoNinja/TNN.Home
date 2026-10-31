#include "test.h"
#include "platform/platform.h"

void platform_test_time(void)
{
    const u64 before = platform_time_raw_ns();
    platform_wait(0, 0, 2);
    const u64 after = platform_time_raw_ns();

    TEST_EXPECT(after > before);
    TEST_EXPECT(after - before > 1 * TIME_NS_PER_MS && after - before < 1 * TIME_NS_PER_S);
}

void platform_test_time_mono(void)
{
    const u64 before = platform_time_mono_ns();
    platform_wait(0, 0, 2);
    const u64 after = platform_time_mono_ns();
    TEST_EXPECT(after > before);
    TEST_EXPECT(after - before > 1 * TIME_NS_PER_MS && after - before < 1 * TIME_NS_PER_S);
}