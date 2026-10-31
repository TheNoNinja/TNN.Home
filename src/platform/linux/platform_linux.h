#ifndef PLATFORM_LINUX_H
#define PLATFORM_LINUX_H

#include "platform/platform_network.h"

int platform_linux_fd(PlatformHandle h);
PlatformHandle platform_linux_handle(int fd);
b32 platform_linux_failed(void);

#endif