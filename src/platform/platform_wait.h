#ifndef PLATFORM_WAIT_H
#define PLATFORM_WAIT_H

#include "platform/platform_network.h"

#define PLATFORM_WAIT_MAX_HANDLES 64

typedef struct PlatformWatch PlatformWatch;
struct PlatformWatch
{
    PlatformHandle handle;
    b32 should_write;
    b32 is_readable;
    b32 is_writable;
};

b32 platform_wait(PlatformWatch *watches, u32 count, int timeout_ms);

#endif