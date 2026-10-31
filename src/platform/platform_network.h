#ifndef PLATFORM_NETWORK_H
#define PLATFORM_NETWORK_H

#include "base/base.h"

typedef struct PlatformHandle PlatformHandle;
struct PlatformHandle
{
    u64 value;
};

const char *platform_error(void);

#endif