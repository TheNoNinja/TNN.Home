#include "base/base_log.h"

#include <stdarg.h>
#include <stdio.h>

void log_line(const char *level, const char *file, const int line, const char *format, ...)
{
#ifndef BASE_LOG_DISABLE
    char message[LOG_MESSAGE_MAX];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    fprintf(stderr, "[%s] %s:%d: %s\n", level, file, line, message);
#else
    UNUSED(level);
    UNUSED(file);
    UNUSED(line);
    UNUSED(format);
#endif
}