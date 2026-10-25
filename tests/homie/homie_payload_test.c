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

void homie_test_state(void)
{
    TEST_EXPECT(homie_state_parse(STR("init")) == HOMIE_STATE_INIT);
    TEST_EXPECT(homie_state_parse(STR("ready")) == HOMIE_STATE_READY);
    TEST_EXPECT(homie_state_parse(STR("lost")) == HOMIE_STATE_LOST);
    TEST_EXPECT(homie_state_parse(STR("Ready")) == HOMIE_STATE_NONE);
    TEST_EXPECT(homie_state_parse(STR_EMPTY) == HOMIE_STATE_NONE);
    TEST_EXPECT_STR(homie_state_name(HOMIE_STATE_SLEEPING), STR("sleeping"));
}

void homie_test_datatype(void)
{
    TEST_EXPECT(homie_datatype_parse(STR("boolean")) == HOMIE_DATATYPE_BOOLEAN);
    TEST_EXPECT(homie_datatype_parse(STR("json")) == HOMIE_DATATYPE_JSON);
    TEST_EXPECT(homie_datatype_parse(STR("bool")) == HOMIE_DATATYPE_NONE);
    TEST_EXPECT(homie_datatype_parse(STR("Float")) == HOMIE_DATATYPE_NONE);
    TEST_EXPECT(homie_datatype_parse(STR("none")) == HOMIE_DATATYPE_NONE);
    TEST_EXPECT_STR(homie_datatype_name(HOMIE_DATATYPE_DURATION), STR("duration"));
}

void homie_test_value_keep(void)
{
    HomieValue v = {0};
    TEST_EXPECT(!v.is_known);
    TEST_EXPECT_STR(homie_value_str(&v), STR_EMPTY);

    u8 bytes[] = "true";
    TEST_EXPECT(homie_value_set(&v, (str){ bytes, sizeof(bytes) - 1 }));
    bytes[0] = 'X';
    TEST_EXPECT(v.is_known);
    TEST_EXPECT_STR(homie_value_str(&v), STR("true"));

    TEST_EXPECT(homie_value_set(&v, STR_EMPTY));
    TEST_EXPECT(!v.is_known);
    TEST_EXPECT_STR(homie_value_str(&v), STR_EMPTY);

    u8 tomany[HOMIE_VALUE_MAX + 1];
    memset(tomany, 'a', sizeof(tomany));
    TEST_EXPECT(homie_value_set(&v, (str){ tomany, sizeof(tomany) - 1 }) && v.length == HOMIE_VALUE_MAX);
    TEST_EXPECT(!homie_value_set(&v, (str){ tomany, sizeof(tomany) }));
    TEST_EXPECT(!v.is_known);
}