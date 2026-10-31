#ifndef PLATFORM_TICKER_H
#define PLATFORM_TICKER_H

#include "platform/platform_network.h"

b32 platform_ticker_start(u64 interval_ns, PlatformHandle *h);
b32 platform_ticker_take(PlatformHandle h, u64 *ticks);

#endif