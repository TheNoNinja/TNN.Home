#include "test.h"

internal const TestCase test_cases[] = {
    TEST_LIST_PURE(TEST_CASE)
};

void app_main(void)
{
    test_run(test_cases, (u32)ARRAY_COUNT(test_cases));
}