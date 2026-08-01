#include "base/base_assert.h"

#include <stdio.h>

_Noreturn void assert_fail(const char *condition, const char *file, const int line) {
    fflush(stdout);
    fprintf(stderr, "%s:%d: ASSERT failed: %s\n", file, line, condition);
    __builtin_trap();
}