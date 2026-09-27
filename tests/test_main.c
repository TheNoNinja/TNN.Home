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
    { .name = "base time",                           .function = base_test_time },
    { .name = "base time mono",                      .function = base_test_time_mono },
    { .name = "base str equal",                      .function = base_test_str_equal },
    { .name = "base str cut",                        .function = base_test_str_cut },
    { .name = "base str from cstr",                  .function = base_test_str_from_cstr },
    { .name = "base str to u64",                     .function = base_test_str_to_u64 },

    { .name = "histogram index",                     .function = histogram_test_index },
    { .name = "histogram bound",                     .function = histogram_test_bounds },
    { .name = "histogram percentiles",               .function = histogram_test_percentiles },

    { .name = "homie topic",                         .function = homie_test_topic },
    { .name = "homie invalid topic",                 .function = homie_test_topic_invalid },
    { .name = "homie topic build",                   .function = homie_test_topic_build },
    { .name = "homie id",                            .function = homie_test_id },
    { .name = "homie state",                         .function = homie_test_state },
    { .name = "homie datatype",                      .function = homie_test_datatype },
    { .name = "homie description",                   .function = homie_test_description },
    { .name = "homie description invalid",           .function = homie_test_description_invalid },
    { .name = "homie value",                         .function = homie_test_value },
    { .name = "homie value bool",                    .function = homie_test_value_bool },
    { .name = "homie value integer",                 .function = homie_test_value_integer },
    { .name = "homie value float",                   .function = homie_test_value_float },
    { .name = "homie value json",                    .function = homie_test_value_json },

    { .name = "http request",                        .function = http_test_request },
    { .name = "http request peices",                 .function = http_test_request_peices },
    { .name = "http request line",                   .function = http_test_request_line },
    { .name = "http request headers",                .function = http_test_request_headers },

    { .name = "json object",                         .function = json_test_object },
    { .name = "json escape",                         .function = json_test_escape },
    { .name = "json overflow",                       .function = json_test_overflow },
    { .name = "json read",                           .function = json_test_read },
    { .name = "json read numbers",                   .function = json_test_read_numbers },
    { .name = "json read booleans",                  .function = json_test_read_booleans },
    { .name = "json read skip",                      .function = json_test_read_skip },
    { .name = "json read invalid",                   .function = json_test_read_invalid },

    { .name = "loop timeout",                        .function = loop_test_timeout },
    { .name = "loop timer fires",                    .function = loop_test_timer_fires },
    { .name = "loop pipe",                           .function = loop_test_pipe },
    { .name = "loop signal",                         .function = loop_test_signal },
    { .name = "loop unwatch",                        .function = loop_test_unwatch },

    { .name = "mqtt string golden bytes",            .function = mqtt_test_str },
    { .name = "mqtt u64 golden bytes",               .function = mqtt_test_u64 },
    { .name = "mqtt varint golden bytes",            .function = mqtt_test_varint },
    { .name = "mqtt varint overflow",                .function = mqtt_test_varint_too_big },
    { .name = "mqtt connect golden bytes",           .function = mqtt_test_connect },
    { .name = "mqtt connect with will golden bytes", .function = mqtt_test_connect_will },
    { .name = "mqtt subscribe golden bytes",         .function = mqtt_test_subscribe },
    { .name = "mqtt unsubscribe golden bytes",       .function = mqtt_test_unsubscribe },
    { .name = "mqtt pingreq golden bytes",           .function = mqtt_test_pingreq },
    { .name = "mqtt read str",                       .function = mqtt_test_read_str },
    { .name = "mqtt read past end",                  .function = mqtt_test_read_past_end },
    { .name = "mqtt read varint",                    .function = mqtt_test_read_varint },
    { .name = "mqtt read varint overflow",           .function = mqtt_test_read_varint_too_long },
    { .name = "mqtt read fixed header",              .function = mqtt_test_read_fixed_header },
    { .name = "mqtt read publish",                   .function = mqtt_test_read_publish },
    { .name = "mqtt stream split packet",            .function = mqtt_test_stream_split },
    { .name = "mqtt stream several packets",         .function = mqtt_test_stream_burst },
    { .name = "mqtt stream oversized",               .function = mqtt_test_stream_oversized },
    { .name = "mqtt stream malformed",               .function = mqtt_test_stream_malformed },
    { .name = "mqtt client send",                    .function = mqtt_test_client_send },
    { .name = "mqtt client next",                    .function = mqtt_test_client_next },
    { .name = "mqtt client refused",                 .function = mqtt_test_client_refused },
    { .name = "mqtt client unsubscribe",             .function = mqtt_test_client_unsubscribe },
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