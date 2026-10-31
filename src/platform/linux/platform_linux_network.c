#include "platform/platform_network.h"
#include "platform/linux/platform_linux.h"

#include <errno.h>
#include <string.h>

int platform_linux_fd(const PlatformHandle h)
{
    return (int)h.value - 1;
}

PlatformHandle platform_linux_handle(const int fd)
{
    return (PlatformHandle) { (u64)fd + 1 };
}

internal int platform_last_error;

b32 platform_linux_failed(void)
{
    platform_last_error = errno;
    return 0;
}

const char *platform_error(void)
{
    return platform_last_error != 0 ? strerror(platform_last_error) : 0;
}