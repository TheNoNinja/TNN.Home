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

void platform_test_socket_pair(void)
{
    PlatformHandle ends[2];
    TEST_EXPECT(platform_socket_pair(ends) && ends[0].value != 0 && ends[1].value != 0);

    u8 bytes[8];
    const str space = { bytes, sizeof(bytes) };
    TEST_EXPECT(platform_receive(ends[1], space) == PLATFORM_NOTHING);

    TEST_EXPECT(platform_send(ends[0], STR("abc")) == 3);
    PlatformWatch watch = { .handle = ends[1] };
    TEST_EXPECT(platform_wait(&watch, 1, 1000) && watch.is_readable);
    TEST_EXPECT(platform_receive(ends[1], space) == 3);
    const str got = { bytes, 3 };
    TEST_EXPECT_STR(got, STR("abc"));

    platform_close(&ends[0]);
    TEST_EXPECT(ends[0].value == 0);
    TEST_EXPECT(platform_receive(ends[1], space) == 0);
    TEST_EXPECT(platform_send(ends[1], STR("X")) == PLATFORM_FAILED && platform_error() != 0);
    platform_close(&ends[1]);
    platform_close(&ends[1]);
}

void platform_test_ticker(void)
{
    PlatformHandle ticker = {0};
    TEST_EXPECT(platform_ticker_start(20 * TIME_NS_PER_MS, &ticker));
    PlatformWatch watch = { .handle = ticker };
    TEST_EXPECT(platform_wait(&watch, 1, 1000) && watch.is_readable);
    u64 ticks = 0;
    TEST_EXPECT(platform_ticker_take(ticker, &ticks) && ticks >= 1);
    TEST_EXPECT(platform_ticker_take(ticker, &ticks) && ticks == 0);
    platform_close(&ticker);
}