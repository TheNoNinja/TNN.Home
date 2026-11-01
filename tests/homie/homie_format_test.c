#include "test.h"
#include "homie/homie.h"

#include <string.h>

void homie_test_payload(void)
{
    TEST_EXPECT(!homie_payload_valid(HOMIE_DATATYPE_NONE, STR("true")));
    TEST_EXPECT(homie_payload_valid(HOMIE_DATATYPE_STRING, STR("hello world")));
    TEST_EXPECT(homie_payload_valid(HOMIE_DATATYPE_ENUM, STR("low")));
    TEST_EXPECT(homie_payload_valid(HOMIE_DATATYPE_COLOR, STR("rgb,255,0,0")));
    TEST_EXPECT(homie_payload_valid(HOMIE_DATATYPE_DATETIME, STR("1999-10-23T07:00:00Z")));
    TEST_EXPECT(homie_payload_valid(HOMIE_DATATYPE_DURATION, STR("PT12M30S")));
    TEST_EXPECT(!homie_payload_valid(HOMIE_DATATYPE_STRING, STR_EMPTY));
    TEST_EXPECT(!homie_payload_valid(HOMIE_DATATYPE_DURATION, STR_EMPTY));
}

void homie_test_payload_bool(void)
{
    TEST_EXPECT(homie_payload_valid(HOMIE_DATATYPE_BOOLEAN, STR("true")));
    TEST_EXPECT(homie_payload_valid(HOMIE_DATATYPE_BOOLEAN, STR("false")));
    const str not_booleans[] = { STR("True"), STR("FALSE"), STR("1"), STR("tru"), STR("true "), STR("maybe") };
    for (u32 i = 0; i < ARRAY_COUNT(not_booleans); i++)
    {
        TEST_EXPECT(!homie_payload_valid(HOMIE_DATATYPE_BOOLEAN, not_booleans[i]));
    }
}

void homie_test_payload_integer(void)
{
    const str integers[] = { STR("0"), STR("42"), STR("-5"), STR("007"), STR("-0"), STR("9223372036854775807"), STR("-9223372036854775808") };
    for (u32 i = 0; i < ARRAY_COUNT(integers); i++)
    {
        TEST_EXPECT(homie_payload_valid(HOMIE_DATATYPE_INTEGER, integers[i]));
    }
    const str not_integers[] = { STR("21.5"), STR("1e3"), STR("+5"), STR(" 5"), STR("-"), STR("5-"), STR("--5"), STR("9223372036854775808"), STR("-9223372036854775809") };
    for (u32 i = 0; i < ARRAY_COUNT(not_integers); i++)
    {
        TEST_EXPECT(!homie_payload_valid(HOMIE_DATATYPE_INTEGER, not_integers[i]));
    }
}

void homie_test_payload_float(void)
{
    const str floats[] = { STR("21.5"), STR("-0.25"), STR("21"), STR("1e3"), STR("1.5E-3"), STR("-7e-2"), STR("007.50") };
    for (u32 i = 0; i < ARRAY_COUNT(floats); i++)
    {
        TEST_EXPECT(homie_payload_valid(HOMIE_DATATYPE_FLOAT, floats[i]));
    }

    const str not_floats[] = { STR(".5"), STR("5."), STR("-"), STR("1e"), STR("1e-"), STR("1e+3"), STR("+1"), STR("1.2.3"), STR("1,5"), STR(" 1"), STR("1 "), STR("NaN"), STR("Infinity"), STR("0x10"), STR_EMPTY };
    for (u32 i = 0; i < ARRAY_COUNT(not_floats); i++)
    {
        TEST_EXPECT(!homie_payload_valid(HOMIE_DATATYPE_FLOAT, not_floats[i]));
    }
}

void homie_test_payload_json(void)
{
    const str jsons[] = { STR("{}"), STR("[]"), STR("{\"on\":true,\"level\":[1,2.5]}"), STR(" [ 1 ] ") };
    for (u32 i = 0; i < ARRAY_COUNT(jsons); i++)
    {
        TEST_EXPECT(homie_payload_valid(HOMIE_DATATYPE_JSON, jsons[i]));
    }

    const str not_jsons[] = { STR("true"), STR("21"), STR("\"text\""), STR("null"), STR("{"), STR("[1,]"),  STR("{} x"), STR_EMPTY };
    for (u32 i = 0; i < ARRAY_COUNT(not_jsons); i++)
    {
        TEST_EXPECT(!homie_payload_valid(HOMIE_DATATYPE_JSON, not_jsons[i]));
    }
}

void homie_test_format_range(void)
{
    HomieFormat f = {0};
    TEST_EXPECT(homie_format_parse(HOMIE_DATATYPE_INTEGER, STR("0:100:5"), &f));
    TEST_EXPECT(f.has_min && f.has_max && f.has_step);
    TEST_EXPECT(f.integer_min == 0 && f.integer_max == 100 && f.integer_step == 5);
    TEST_EXPECT_STR(homie_format_str(&f), STR("0:100:5"));

    TEST_EXPECT(homie_format_parse(HOMIE_DATATYPE_INTEGER, STR(":10"), &f));
    TEST_EXPECT(!f.has_min && f.has_max && f.integer_max == 10 && !f.has_step);
    TEST_EXPECT(homie_format_parse(HOMIE_DATATYPE_INTEGER, STR("-5:"), &f));
    TEST_EXPECT(f.has_min && f.integer_min == -5 && !f.has_max  && !f.has_step);
    TEST_EXPECT(homie_format_parse(HOMIE_DATATYPE_INTEGER, STR_EMPTY, &f));
    TEST_EXPECT(!f.has_min && !f.has_max  && !f.has_step);
    TEST_EXPECT_STR(homie_format_str(&f), STR_EMPTY);

    TEST_EXPECT(homie_format_parse(HOMIE_DATATYPE_FLOAT, STR("10.125:15.5:0.25"), &f));
    TEST_EXPECT(f.has_min && f.has_max && f.has_step);
    TEST_EXPECT(f.float_min == 10.125 && f.float_max == 15.5 && f.float_step == 0.25);
    TEST_EXPECT_STR(homie_format_str(&f), STR("10.125:15.5:0.25"));
}

void homie_test_format_invalid(void)
{
    HomieFormat f = {0};
    const str invalid_ranges[] = {
        STR("5"),
        STR("10:0"),
        STR("0:10:0"),
        STR("0:10:-1"),
        STR("0:1.5"),
        STR("a:b"),
        STR(" 0:10"),
        STR("0:10:2:1")
    };
    for (u32 i = 0; i < ARRAY_COUNT(invalid_ranges); i++)
    {
        TEST_EXPECT(!homie_format_parse(HOMIE_DATATYPE_INTEGER, invalid_ranges[i], &f));
    }
    TEST_EXPECT(!homie_format_parse(HOMIE_DATATYPE_FLOAT, STR("0:1e999"), &f));
    TEST_EXPECT(!homie_format_parse(HOMIE_DATATYPE_NONE, STR_EMPTY, &f));
    u8 many[HOMIE_FORMAT_MAX + 1];
    memset(many, 'a', sizeof(many));
    TEST_EXPECT(homie_format_parse(HOMIE_DATATYPE_STRING, (str){ many, HOMIE_FORMAT_MAX }, &f));
    TEST_EXPECT(!homie_format_parse(HOMIE_DATATYPE_STRING, (str){ many, sizeof(many) }, &f));
}