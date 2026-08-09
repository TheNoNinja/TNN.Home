#ifndef BASE_LOG_H
#define BASE_LOG_H

#define LOG_ERROR(...) log_line("error", __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) log_line("warn", __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) log_line("info", __FILE__, __LINE__, __VA_ARGS__)

#define LOG_MESSAGE_MAX 256

void log_line(const char *level, const char *file, int line, const char *format, ...) __attribute__((format(printf, 4, 5)));

#endif