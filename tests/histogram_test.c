#include "test.h"
#include "histogram/histogram.h"

#include "base/base_log.h"

void histogram_test_index(void)
{
    TEST_EXPECT(histogram_index(0) == 0);
    TEST_EXPECT(histogram_index(63) == 63 && histogram_index(64) == 64);
    TEST_EXPECT(histogram_index(127) == 127);

    TEST_EXPECT(histogram_index(128) == 128 && histogram_index(129) == 128);
    TEST_EXPECT(histogram_index(130) == 129);
    TEST_EXPECT(histogram_index(255) == 191);
    TEST_EXPECT(histogram_index(256) == 192 && histogram_index(259) == 192);
    TEST_EXPECT(histogram_index(260) == 193);

    TEST_EXPECT(histogram_index(1000000) == 954);

    TEST_EXPECT(histogram_index(UINT64_MAX) == HISTOGRAM_BUCKET_COUNT - 1);
}

void histogram_test_bounds(void)
{
    TEST_EXPECT(histogram_bucket_low(954) == 999424 && histogram_bucket_high(954) == 1007615);
    TEST_EXPECT(histogram_bucket_high(HISTOGRAM_BUCKET_COUNT - 1) == UINT64_MAX);

    b32 inside = 1;
    b32 narrow = 1;
    for (u64 value = 0; value < UINT64_MAX / 2; value += value / 7 + 1)
    {
        const u32 index = histogram_index(value);
        const u64 low = histogram_bucket_low(index);
        const u64 high = histogram_bucket_high(index);
        inside = inside && low <= value && value <= high;
        narrow = narrow && (high - low) * HISTOGRAM_SUB_COUNT <= low;
    }
    TEST_EXPECT(inside);
    TEST_EXPECT(narrow);
}

void histogram_test_percentiles(void)
{
    Histogram h = histogram_create();
    TEST_EXPECT(h.count == 0 && histogram_percentile(&h, 50.0) == 0);

    for (u64 value = 1; value <= 100; value++)
    {
        histogram_record(&h, value);
    }
    TEST_EXPECT(h.count == 100 && h.min == 1 && h.max == 100);
    TEST_EXPECT(histogram_percentile(&h, 50.0) == 50);
    TEST_EXPECT(histogram_percentile(&h, 99.0) == 99);
    TEST_EXPECT(histogram_percentile(&h, 99.9) == 100);
    TEST_EXPECT(histogram_percentile(&h, 100.0) == 100);

    h = histogram_create();
    histogram_record(&h, 1000000);
    histogram_record(&h, 2000000);
    TEST_EXPECT(histogram_percentile(&h, 50.0) == 1007615);
    TEST_EXPECT(histogram_percentile(&h, 100.0) == 2000000);
}