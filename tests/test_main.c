#include "test.h"

internal const TestCase test_cases[] = {
    TEST_LIST(TEST_CASE)
};

int main(void)
{
    u32 failed = test_run(test_cases, (u32)ARRAY_COUNT(test_cases));
    return failed == 0 ? 0 : 1;
}