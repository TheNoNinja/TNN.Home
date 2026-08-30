#ifndef BASE_TIME_H
#define BASE_TIME_H

#include "base/base_types.h"

#define TIME_NS_PER_US 1000
#define TIME_NS_PER_MS 1000000
#define TIME_NS_PER_S  1000000000

u64 time_now_raw_ns(void);
u64 time_now_mono_ns(void);

#endif