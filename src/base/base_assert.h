#ifndef BASE_ASSERT_H
#define BASE_ASSERT_H

#define ASSERT(condition) ((condition) ? (void)0 : assert_fail(#condition, __FILE__, __LINE__))

_Noreturn void assert_fail(const char *condition, const char *file, int line);

#endif
