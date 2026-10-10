#include "test.h"

#include <stdio.h>
#include <string.h>

typedef void TestFunction(void);
typedef struct TestCase TestCase;
struct TestCase
{
    const char *name;
    TestFunction *function;
};

#define TEST_CASE(function, name) { name, function },

internal const TestCase test_cases[] = {
    TEST_LIST(TEST_CASE)
};

internal const char *test_current_name;
internal b32 test_current_failed;

internal void test_fail(const char *file, const int line)
{
    if (!test_current_failed)
    {
        printf("FAIL\t%s\n", test_current_name);
        test_current_failed = 1;
    }
    printf("\t  %s:%d: expected", file, line);
}

void test_expect(const b32 ok, const char *condition, const char *file, const int line)
{
    if (!ok)
    {
        test_fail(file, line);
        printf(" %s\n", condition);
    }
}

internal void test_print_hex(const u8* data, const u64 length)
{
    for (u64 i = 0; i < length; i++)
    {
        printf (" %02x", data[i]);
    }
}

void test_expect_bytes(const u8 *data, const u64 length, const u8 *expected, const u64 expected_length, const char *file, const int line)
{
    if (length == expected_length && memcmp(data, expected, length) == 0)
    {
        return;
    }

    test_fail(file, line);
    test_print_hex(expected, expected_length);
    printf(", got");
    test_print_hex(data, length);
    printf("\n");
}

internal void test_print_str(const str s)
{
    printf("\"");
    for(u64 i = 0; i < s.length; i++)
    {
        const u8 c = s.data[i];
        if (c == '\r')
        {
            printf("\\r");
        }
        else if (c == '\n')
        {
            printf("\\n");
        }
        else if (c < 0x20 || c == 0x7F)
        {
            printf("\\x%02x", c);
        }
        else
        {
            putchar(c);
        }
    }
    printf("\"");
}

void test_expect_str(const str actual, const str expected, const char *text, const char* file, const int line)
{
    if (str_equal(actual, expected))
    {
        return;
    }
    test_fail(file, line);
    printf("  %s to be ", text);
    test_print_str(expected);
    printf(", got ");
    test_print_str(actual);
    printf("\n");
}

int main(void)
{
    const u32 count = ARRAY_COUNT(test_cases);
    u32 failed = 0;
    for (u32 i = 0; i < count; i++)
    {
        test_current_name = test_cases[i].name;
        test_current_failed = 0;
        test_cases[i].function();

        if (test_current_failed)
        {
            failed += 1;
        }
        else
        {
            printf("PASS\t%s\n", test_current_name);
        }
    }
    printf("\n%u of %u tests passed\n", count - failed, count);
    return failed == 0 ? 0 : 1;
}