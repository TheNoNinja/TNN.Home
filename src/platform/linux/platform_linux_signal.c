#include "platform/platform_signal.h"

#include <signal.h>
#include <string.h>


internal volatile sig_atomic_t platform_signal_received;

internal void platform_signal_handler(const int number)
{
    platform_signal_received = number;
}

void platform_signals_catch(void)
{
    struct sigaction action = {0};
    action.sa_handler = platform_signal_handler;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, 0);
    sigaction(SIGTERM, &action, 0);
}

int platform_signal_take(void)
{
    const int number = (int)platform_signal_received;
    platform_signal_received = 0;
    return number;
}

const char *plarform_signal_name(const int number)
{
    return strsignal(number);
}