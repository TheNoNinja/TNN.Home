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

internal const TestCase test_cases[] = {
    { .name = "base arena push",                     .function = base_test_arena_push },
    { .name = "base arena full",                     .function = base_test_arena_full },
    { .name = "base arena pop",                      .function = base_test_arena_pop },

    { .name = "mqtt string golden bytes",            .function = mqtt_test_str },
    { .name = "mqtt varint golden bytes",            .function = mqtt_test_varint },
    { .name = "mqtt varint overflow",                .function = mqtt_test_varint_too_big },
    { .name = "mqtt connect golden bytes",           .function = mqtt_test_connect },
    { .name = "mqtt connect with will golden bytes", .function = mqtt_test_connect_will },
    { .name = "mqtt subscribe golden bytes",         .function = mqtt_test_subscribe },
    { .name = "mqtt pingreq golden bytes",           .function = mqtt_test_pingreq },
    { .name = "mqtt read str",                       .function = mqtt_test_read_str },
    { .name = "mqtt read past end",                  .function = mqtt_test_read_past_end },
    { .name = "mqtt read varint",                    .function = mqtt_test_read_varint },
    { .name = "mqtt read varint overflow",           .function = mqtt_test_read_varint_too_long },
    { .name = "mqtt read fixed header",              .function = mqtt_test_read_fixed_header },
    { .name = "mqtt read publish",                   .function = mqtt_test_read_publish },
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