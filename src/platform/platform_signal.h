#ifndef PLATFORM_SIGNAL_H
#define PLATFORM_SIGNAL_H

#include "base/base.h"

void platform_signals_catch(void);
int platform_signal_take(void);
const char *platform_signal_name(int number);

#endif