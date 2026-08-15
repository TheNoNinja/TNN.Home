#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "base/base_types.h"

#define HISTOGRAM_SUB_BITS 6
#define HISTOGRAM_SUB_COUNT (1u << HISTOGRAM_SUB_BITS)
#define HISTOGRAM_BUCKET_COUNT ((64 - HISTOGRAM_SUB_BITS + 1) * HISTOGRAM_SUB_COUNT)

typedef struct Histogram Histogram;
struct Histogram
{
    u64 counts[HISTOGRAM_BUCKET_COUNT];
    u64 count;
    u64 min;
    u64 max;
};

Histogram histogram_create(void);
void histogram_record(Histogram *h, u64 value);

u64 histogram_percentile(const Histogram *h, f64 percent);

void histogram_print(const Histogram *h, const char *name);

u32 histogram_index(u64 value);
u64 histogram_bucket_low(u32 index);
u64 histogram_bucket_high(u32 index);

#endif