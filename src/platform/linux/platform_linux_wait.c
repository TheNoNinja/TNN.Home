#define _POSIX_C_SOURCE 200809L
#include "platform/platform_wait.h"
#include "platform/linux/platform_linux.h"

#include <errno.h>
#include <poll.h>

b32 platform_wait(PlatformWatch *watches, const u32 count, const int timeout_ms)
{
    ASSERT(count <= PLATFORM_WAIT_MAX_HANDLES);

    struct pollfd fds[PLATFORM_WAIT_MAX_HANDLES];
    for (u32 i = 0; i < count; i++)
    {
        fds[i].fd = platform_linux_fd(watches[i].handle);
        fds[i].events = (short)(POLLIN | (watches[i].should_write ? POLLOUT : 0));
        fds[i].revents = 0;
        watches[i].is_readable = 0;
        watches[i].is_writable = 0;
    }
    const int ready = poll(fds, count, timeout_ms);

    if(ready < 0)
    {
        return errno == EINTR ? 1 : platform_linux_failed();
    }

    for(u32 i = 0; i < count; i++)
    {
        const short gone = POLLHUP | POLLERR;
        watches[i].is_readable = (fds[i].revents & (POLLIN | gone)) != 0;
        watches[i].is_writable = watches[i].should_write && (fds[i].revents & (POLLOUT | gone)) != 0;
    }

    return 1;
}