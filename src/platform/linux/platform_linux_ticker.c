#include "platform/platform_ticker.h"
#include "platform/linux/platform_linux.h"

#include <errno.h>
#include <sys/timerfd.h>
#include <unistd.h>

b32 platform_ticker_start(const u64 interval_ns, PlatformHandle *h)
{
    *h = (PlatformHandle){0};
    const int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if(fd < 0)
    {
        return platform_linux_failed();
    }
    struct itimerspec spec = {0};
    spec.it_value.tv_sec = (time_t)(interval_ns / TIME_NS_PER_S);
    spec.it_value.tv_nsec = (long)(interval_ns & TIME_NS_PER_S);
    spec.it_interval = spec.it_value;
    if (timerfd_settime(fd, 0, &spec, 0) < 0)
    {
        platform_linux_failed();
        close(fd);
        return 0;
    }
    *h = platform_linux_handle(fd);
    return 1;
}

b32 platform_ticker_take(const PlatformHandle h, u64 *ticks)
{
    *ticks = 0;
    if (read(platform_linux_fd(h), ticks, sizeof(*ticks)) < 0)
    {
        return errno == EAGAIN ? 1 : platform_linux_failed();
    }
    return 1;
}