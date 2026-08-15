#include "histogram/histogram.h"
#include "base/base_assert.h"

Histogram histogram_create(void)
{
    Histogram h = {0};
    h.min = UINT64_MAX;
    return h;
}

u32 histogram_index(const u64 value)
{
    if (value < HISTOGRAM_SUB_COUNT)
    {
        return (u32)value;
    }

    const u32 top = 63 - (u32)__builtin_clzll(value);
    const u32 shift = top - HISTOGRAM_SUB_BITS;
    const u32 sub = (u32)(value >> shift) - HISTOGRAM_SUB_COUNT;
    return (shift + 1) * HISTOGRAM_SUB_COUNT + sub;
}

u64 histogram_bucket_low(const u32 index)
{
    if (index < HISTOGRAM_SUB_COUNT)
    {
        return index;
    }

    const u32 shift = index / HISTOGRAM_SUB_COUNT - 1;
    const u64 sub = index % HISTOGRAM_SUB_COUNT;
    return (HISTOGRAM_SUB_COUNT + sub) << shift;
}

u64 histogram_bucket_high(const u32 index)
{
    return histogram_bucket_low(index + 1) - 1;
}

void histogram_record(Histogram *h, const u64 value)
{
    h->counts[histogram_index(value)] += 1;
    h->count += 1;
    if (value < h->min)
    {
        h->min = value;
    }
    if (value > h->max)
    {
        h->max = value;
    }
}

u64 histogram_percentile(const Histogram *h, f64 percent)
{
    percent = percent < 0.0 ? 0.0 : percent;
    percent = percent > 100.0 ? 100.0 : percent;
    if (h->count == 0)
    {
        return 0;
    }

    u64 wanted = (u64)(percent / 100.0 * (f64)h->count + 0.5);
    if (wanted < 1)
    {
        wanted = 1;
    }

    u64 seen = 0;
    for (u32 i = 0; i < HISTOGRAM_BUCKET_COUNT; i++)
    {
        seen += h->counts[i];
        if (seen >= wanted)
        {
            const u64 high = histogram_bucket_high(i);
            return high < h->max ? high : h->max;
        }
    }
    return h->max;

}