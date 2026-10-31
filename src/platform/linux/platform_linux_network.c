#include "platform/platform_network.h"
#include "platform/linux/platform_linux.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PLATFORM_LISTEN_BACKLOG 16

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

internal void platform_unblock(const int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

internal void platform_no_delay(const int fd)
{
    const int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

internal b32 platform_address(const char *ip, const u16 port, struct sockaddr_in *address)
{
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &address->sin_addr) != 1)
    {
        errno = EINVAL;
        return 0;
    }
    return 1;
}

b32 platform_tcp_connect(const char *ip, const u16 port, PlatformHandle *h)
{
    *h = (PlatformHandle){0};
    struct sockaddr_in broker;
    if (!platform_address(ip, port, &broker))
    {
        return platform_linux_failed();
    }
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        return platform_linux_failed();
    }
    platform_no_delay(fd);
    if (connect(fd, (struct sockaddr *)&broker, sizeof(broker)) < 0)
    {
        platform_linux_failed();
        close(fd);
        return 0;
    }
    platform_unblock(fd);
    *h = platform_linux_handle(fd);
    return 1;
}

b32 platform_tcp_listen(const char *ip, const u16 port, PlatformHandle *h)
{
    *h = (PlatformHandle){0};
    struct sockaddr_in address;
    if (!platform_address(ip, port, &address))
    {
        return platform_linux_failed();
    }
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        return platform_linux_failed();
    }

    const int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0 || listen(fd, PLATFORM_LISTEN_BACKLOG) < 0)
    {
        platform_linux_failed();
        close(fd);
        return 0;
    }
    platform_unblock(fd);
    *h = platform_linux_handle(fd);
    return 1;
}

b32 platform_tcp_accept(const PlatformHandle listener, PlatformHandle *h)
{
    *h = (PlatformHandle){0};
    const int fd = accept(platform_linux(fd), 0, 0);
    if (fd < 0)
    {
        if (errno == EAGAIN | errno == EINTR)
        {
            platform_last_error = 0;
            return 0;
        }
        return platform_linux_failed;
    }
    platform_unblock(fd);
    platform_no_delay(fd);
    *h = platform_linux_handle(fd);
    return 1;
}

i64 platform_send(const PlatformHandle h, const str bytes)
{
    const ssize_t sent= send(platform_linux_fd(h), bytes.data, bytes.length, MSG_NOSIGNAL);
    if (sent >= 0)
    {
        return (i64)sent;
    }
    if (errno == EAGAIN || errno == EINTR)
    {
        return 0;
    }
    platform_linux_failed();
    return PLATFORM_FAILED;
}

i64 platform_receive(const PlatformHandle h, const str space)
{
    const ssize_t received = read(platform_linux_fd(h), space.data, space.length);
    if (received >= 0)
    {
        return (i64)received;
    }
    if (errno == EAGAIN || errno == EINTR)
    {
        return PLATFORM_NOTHING;
    }
    platform_linux_failed();
    return PLATFORM_FAILED;
}

void platform_close(PlatformHandle *h)
{
    if (h->value != 0)
    {
        close(platform_linux_fd(*h));
    }
    *h = (PlatformHandle){0};
}

PlatformHandle platform_stdin(void)
{
    return platform_linux_handle(STDIN_FILENO);
}

b32 platform_socket_pair(PlatformHandle pair[2])
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
        return platform_linux_failed();
    }
    for (u32 i = 0; i < 2; i++)
    {
        platform_unblock(fds[i]);
        pair[i] = platform_linux_handle(fds[i]);
    }
    return 1;
}