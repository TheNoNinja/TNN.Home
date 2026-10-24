#include "test.h"
#include "homie/homie.h"
#include "json/json.h"

void homie_test_description(void)
{
    const str light = STR("{\"homie\":\"5.0\",\"version\":2,\"name\":\"TNN virtual light\",\"nodes\":{\"light\":{\"name\":\"Light\",\"properties\":{\"on\":{\"name\":\"On\",\"datatype\":\"boolean\",\"settable\":true}}}}}");
    HomieDescription d = {0};
    TEST_EXPECT(homie_description_parse(light, &d));
    TEST_EXPECT(d.version == 2 && d.property_count == 1);
    const HomieProperty *on = &d.properties[0];
    TEST_EXPECT(str_equal(homie_id_str(&on->node), STR("light")) && str_equal(homie_id_str(&on->id), STR("on")));
    TEST_EXPECT(on->datatype == HOMIE_DATATYPE_BOOLEAN && on->is_settable && on->is_retained);

    const str two = STR("{\"homie\":\"5.1\",\"version\":-42,\"children\":[\"a\",\"b\"],\"nodes\":{\"sensor\":{\"properties\":{\"temp\":{\"datatype\":\"float\",\"unit\":\"C\",\"retained\":false},\"Bad\":{\"datatype\":\"integer\"},\"odd\":{\"datatype\":\"bool\"}}},\"switch\":{\"type\":\"relay\",\"properties\":{\"on\":{\"datatype\":\"boolean\",\"settable\":true}}}}}");
    TEST_EXPECT(homie_description_parse(two, &d));
    TEST_EXPECT(d.version == -42 && d.property_count == 2);
    const HomieProperty *temp = &d.properties[0];
    TEST_EXPECT(str_equal(homie_id_str(&temp->id), STR("temp")) && temp->datatype == HOMIE_DATATYPE_FLOAT);
    TEST_EXPECT(!temp->is_settable && !temp->is_retained);
    TEST_EXPECT(str_equal(homie_id_str(&d.properties[1].node), STR("switch")) && d.properties[1].is_settable);

    u8 buffer[1024];
    JsonWriter w = json_writer_create(buffer, sizeof(buffer));
    json_write_object_begin(&w, STR_EMPTY);
    json_write_str(&w, STR("homie"), STR("5.0"));
    json_write_i64(&w, STR("version"), 1);
    json_write_object_begin(&w, STR("nodes"));
    json_write_object_begin(&w, STR("n"));
    json_write_object_begin(&w, STR("properties"));
    for (u32 i = 0; i < HOMIE_PROPERTY_MAX + 1; i++)
    {
        u8 id[2] = { 'p', (u8)('a' + i) };
        json_write_object_begin(&w, (str){ id, sizeof(id) });
        json_write_str(&w, STR("datatype"), STR("string"));
        json_write_object_end(&w);
    }
    json_write_object_end(&w);
    json_write_object_end(&w);
    json_write_object_end(&w);
    json_write_object_end(&w);

    TEST_EXPECT(!w.out.has_overflown);
    TEST_EXPECT(homie_description_parse(writer_to_str(&w.out), &d));
    TEST_EXPECT(d.property_count == HOMIE_PROPERTY_MAX);
    TEST_EXPECT_STR(homie_id_str(&d.properties[15].id), STR("pp"));
}

void homie_test_description_invalid(void)
{
    const str ignored[] = { STR("{\"homie\":\"4.0\",\"version\":1}"), STR("{\"homie\":\"5.0\"}"), STR("{\"homie\":\"5.0\",\"version\":\"2\"}"), STR("{\"homie\":\"5.0\",\"version\":2"), STR_EMPTY };
    HomieDescription d = {0};
    for (u32 i = 0; i < ARRAY_COUNT(ignored); i++)
    {
        TEST_EXPECT(!homie_description_parse(ignored[i], &d));
    }
}

void homie_test_property_find(void)
{
    const str description = STR("{\"homie\":\"5.0\",\"version\":1,\"name\":\"TNN virtual light\",\"nodes\":{\"light\":{\"properties\":{\"on\":{\"datatype\":\"boolean\"}}},\"fan\":{\"properties\":{\"on\":{\"datatype\":\"boolean\"},\"speed\":{\"datatype\":\"integer\"}}}}}");
    HomieDescription d = {0};
    TEST_EXPECT(homie_description_parse(description, &d) && d.property_count == 3);
    TEST_EXPECT(homie_property_find(&d, STR("light"), STR("on")) == 0);
    TEST_EXPECT(homie_property_find(&d, STR("fan"), STR("on")) == 1);
    TEST_EXPECT(homie_property_find(&d, STR("fan"), STR("speed")) == 2);
    TEST_EXPECT(homie_property_find(&d, STR("light"), STR("speed")) == -1);
    TEST_EXPECT(homie_property_find(&d, STR_EMPTY, STR_EMPTY) == -1);
}

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