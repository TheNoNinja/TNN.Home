#include "base/base_assert.h"
#include "base/base_log.h"

#include <stdio.h>

_Noreturn void assert_fail(const char *condition, const char *file, const int line) 
{
    fflush(stdout);
    log_line("error", file, line, "ASSERT failed: %s\n", condition);
    __builtin_trap();
}