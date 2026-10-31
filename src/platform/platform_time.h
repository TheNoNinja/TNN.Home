#ifndef PLATFORM_TIME_H
#define PLATFORM_TIME_H

#include "base/base.h"

u64 platform_time_raw_ns(void);
u64 platform_time_mono_ns(void);

#endif