#ifndef PLATFORM_NETWORK_H
#define PLATFORM_NETWORK_H

#include "base/base.h"

#define PLATFORM_NOTHING (-1)
#define PLATFORM_FAILED  (-2)

typedef struct PlatformHandle PlatformHandle;
struct PlatformHandle
{
    u64 value;
};

const char *platform_error(void);

b32 platform_tcp_connect(const char *ip, u16 port, PlatformHandle *h);
b32 platform_tcp_listen(const char* ip, u16 port, PlatformHandle *h);
b32 platform_tcp_accept(PlatformHandle listener, PlatformHandle *h);

i64 platform_send(PlatformHandle h, str bytes);
i64 platform_receive(PlatformHandle, str space);

void platform_close(PlatformHandle *h);
PlatformHandle platform_stdin(void);
b32 platform_socket_pair(PlatformHandle pair[2]);

#endif