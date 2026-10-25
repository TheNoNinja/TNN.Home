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