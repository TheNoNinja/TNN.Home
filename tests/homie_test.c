#include "test.h"
#include "homie/homie.h"
#include "json/json.h"

#include <string.h>

void homie_test_topic(void)
{
    HomieTopic t = {0};
    TEST_EXPECT(homie_topic_parse(STR("homie/5/tnn-sim-1/$state"), &t));
    TEST_EXPECT(t.kind == HOMIE_TOPIC_STATE);
    TEST_EXPECT_STR(t.domain, STR("homie"));
    TEST_EXPECT_STR(t.device, STR("tnn-sim-1"));
    TEST_EXPECT(t.node.length == 0 && t.property.length == 0);

    TEST_EXPECT(homie_topic_parse(STR("homie/5/tnn-sim-1/light/on"), &t));
    TEST_EXPECT(t.kind == HOMIE_TOPIC_VALUE);
    TEST_EXPECT_STR(t.node, STR("light"));
    TEST_EXPECT_STR(t.property, STR("on"));
    TEST_EXPECT(homie_topic_parse(STR("homie/5/tnn-sim-1/light/on/set"), &t) && t.kind == HOMIE_TOPIC_SET);
    TEST_EXPECT(homie_topic_parse(STR("homie/5/tnn-sim-1/light/on/$target"), &t) && t.kind == HOMIE_TOPIC_TARGET);
    TEST_EXPECT_STR(t.property, STR("on"));
    
    TEST_EXPECT(homie_topic_parse(STR("homie/5/tnn-sim-1/$description"), &t) && t.kind == HOMIE_TOPIC_DESCRIPTION);
    TEST_EXPECT(homie_topic_parse(STR("homie/5/tnn-sim-1/$log/warn"), &t) && t.kind == HOMIE_TOPIC_LOG);
    TEST_EXPECT_STR(t.name, STR("warn"));
    TEST_EXPECT(homie_topic_parse(STR("homie/5/tnn-sim-1/$alert/battery-low"), &t) && t.kind == HOMIE_TOPIC_ALERT);
    TEST_EXPECT_STR(t.name, STR("battery-low"));
    TEST_EXPECT(t.node.length == 0 && t.property.length == 0);

    TEST_EXPECT(homie_topic_parse(STR("test-2/5/a/$description"), &t));
    TEST_EXPECT(str_equal(t.domain, STR("test-2")) && str_equal(t.device, STR("a")) && t.kind == HOMIE_TOPIC_DESCRIPTION);
}

void homie_test_topic_invalid(void)
{
    HomieTopic t = {0};
    TEST_EXPECT(!homie_topic_parse(STR("homie/4/tnn-sim-1/$state"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/tnn-sim-1"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/tnn-sim-1/"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5//$state"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/Tnn-sim-1/$state"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/tnn_sim_1/$state"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/$tnn/$state"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/$broadcast/alarm"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/tnn-sim-1/$state/x"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/tnn-sim-1/$name"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/tnn-sim-1/$log"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/tnn-sim-1/light"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/tnn-sim-1/light/On"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/tnn-sim-1/light/on/$name"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/tnn-sim-1/light/on/set/x"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("tnn/echo-probe"), &t));
    TEST_EXPECT(homie_topic_parse(STR("homie/5/abcdefghijklmnopqrstuvwxyz012345/$state"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/abcdefghijklmnopqrstuvwxyz0123456/$state"), &t));
}

void homie_test_topic_build(void)
{
    const str topics[] = {
        STR("homie/5/tnn-sim-1/$state"),
        STR("homie/5/tnn-sim-1/$description"),
        STR("homie/5/tnn-sim-1/$log/warn"),
        STR("homie/5/tnn-sim-1/$alert/batery-low"),
        STR("homie/5/tnn-sim-1/light/on"),
        STR("homie/5/tnn-sim-1/light/on/$target"),
        STR("homie/5/tnn-sim-1/light/on/set"),
    };

    for (u32 i = 0; i < ARRAY_COUNT(topics); i++)
    {
        HomieTopic t = {0};
        TEST_EXPECT(homie_topic_parse(topics[i], &t));
        u8 buffer[HOMIE_TOPIC_MAX];
        TEST_EXPECT_STR(homie_topic_build(buffer, sizeof(buffer), &t), topics[i]);
    }

    const HomieTopic t = { .kind = HOMIE_TOPIC_VALUE, .domain = STR("homie"), .device = STR("tnn-sim-1"), .node = STR("+"), .property = STR("+") };
    u8 buffer[HOMIE_TOPIC_MAX];
    TEST_EXPECT_STR(homie_topic_build(buffer, sizeof(buffer), &t), STR("homie/5/tnn-sim-1/+/+"));
    TEST_EXPECT_STR(STR(HOMIE_DISCOVERY_FILTER), STR("+/5/+/$state"));

    TEST_EXPECT(homie_topic_build(buffer, 21, &t).length == 21);
    TEST_EXPECT(homie_topic_build(buffer, 20, &t).length == 0);
}

void homie_test_id(void)
{
    u8 bytes[] = "tnn-sim-1";
    const HomieId id = homie_id_copy((str){ bytes, sizeof(bytes) - 1 });
    bytes[0] = 'X';
    TEST_EXPECT_STR(homie_id_str(&id), STR("tnn-sim-1"));
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

void homie_test_description(void)
{
    const str light = STR("{\"homie\":\"5.0\",\"version\":2,\"name\":\"TNN virtual light\",\"nodes\":{\"light\":{\"name\":\"Light\",\"properties\":{\"on\":{\"name\":\"On\",\"datatype\":\"boolean\",\"settable\":true}}}}}");
    HomieDescription d = {0};
    TEST_EXPECT(homie_description_read(light, &d));
    TEST_EXPECT(d.version == 2 && d.property_count == 1);
    const HomieProperty *on = &d.properties[0];
    TEST_EXPECT(str_equal(homie_id_str(&on->node), STR("light")) && str_equal(homie_id_str(&on->id), STR("on")));
    TEST_EXPECT(on->datatype == HOMIE_DATATYPE_BOOLEAN && on->is_settable && on->is_retained);

    const str two = STR("{\"homie\":\"5.1\",\"version\":-42,\"children\":[\"a\",\"b\"],\"nodes\":{\"sensor\":{\"properties\":{\"temp\":{\"datatype\":\"float\",\"unit\":\"C\",\"retained\":false},\"Bad\":{\"datatype\":\"integer\"},\"odd\":{\"datatype\":\"bool\"}}},\"switch\":{\"type\":\"relay\",\"properties\":{\"on\":{\"datatype\":\"boolean\",\"settable\":true}}}}}");
    TEST_EXPECT(homie_description_read(two, &d));
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
    TEST_EXPECT(homie_description_read(writer_to_str(&w.out), &d));
    TEST_EXPECT(d.property_count == HOMIE_PROPERTY_MAX);
    TEST_EXPECT_STR(homie_id_str(&d.properties[15].id), STR("pp"));
}

void homie_test_description_invalid(void)
{
    const str ignored[] = { STR("{\"homie\":\"4.0\",\"version\":1}"), STR("{\"homie\":\"5.0\"}"), STR("{\"homie\":\"5.0\",\"version\":\"2\"}"), STR("{\"homie\":\"5.0\",\"version\":2"), STR_EMPTY };
    HomieDescription d = {0};
    for (u32 i = 0; i < ARRAY_COUNT(ignored); i++)
    {
        TEST_EXPECT(!homie_description_read(ignored[i], &d));
    }
}

void homie_test_property_find(void)
{
    const str description = STR("{\"homie\":\"5.0\",\"version\":1,\"name\":\"TNN virtual light\",\"nodes\":{\"light\":{\"properties\":{\"on\":{\"datatype\":\"boolean\"}}},\"fan\":{\"properties\":{\"on\":{\"datatype\":\"boolean\"},\"speed\":{\"datatype\":\"integer\"}}}}}");
    HomieDescription d = {0};
    TEST_EXPECT(homie_description_read(description, &d) && d.property_count == 3);
    TEST_EXPECT(homie_property_find(&d, STR("light"), STR("on")) == 0);
    TEST_EXPECT(homie_property_find(&d, STR("fan"), STR("on")) == 1);
    TEST_EXPECT(homie_property_find(&d, STR("fan"), STR("speed")) == 2);
    TEST_EXPECT(homie_property_find(&d, STR("light"), STR("speed")) == -1);
    TEST_EXPECT(homie_property_find(&d, STR_EMPTY, STR_EMPTY) == -1);
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

void homie_test_value(void)
{
    TEST_EXPECT(!homie_value_valid(HOMIE_DATATYPE_NONE, STR("true")));    
    TEST_EXPECT(homie_value_valid(HOMIE_DATATYPE_STRING, STR("hello world")));
    TEST_EXPECT(homie_value_valid(HOMIE_DATATYPE_ENUM, STR("low")));
    TEST_EXPECT(homie_value_valid(HOMIE_DATATYPE_COLOR, STR("rgb,255,0,0")));
    TEST_EXPECT(homie_value_valid(HOMIE_DATATYPE_DATETIME, STR("1999-10-23T07:00:00Z")));
    TEST_EXPECT(homie_value_valid(HOMIE_DATATYPE_DURATION, STR("PT12M30S")));
    TEST_EXPECT(!homie_value_valid(HOMIE_DATATYPE_STRING, STR_EMPTY));
    TEST_EXPECT(!homie_value_valid(HOMIE_DATATYPE_DURATION, STR_EMPTY));
}

void homie_test_value_bool(void)
{
    TEST_EXPECT(homie_value_valid(HOMIE_DATATYPE_BOOLEAN, STR("true")));
    TEST_EXPECT(homie_value_valid(HOMIE_DATATYPE_BOOLEAN, STR("false")));
    const str not_booleans[] = { STR("True"), STR("FALSE"), STR("1"), STR("tru"), STR("true "), STR("maybe") };
    for (u32 i = 0; i < ARRAY_COUNT(not_booleans); i++)
    {
        TEST_EXPECT(!homie_value_valid(HOMIE_DATATYPE_BOOLEAN, not_booleans[i]));
    }
}

void homie_test_value_integer(void)
{
    const str integers[] = { STR("0"), STR("42"), STR("-5"), STR("007"), STR("-0"), STR("9223372036854775807"), STR("-9223372036854775808") };
    for (u32 i = 0; i < ARRAY_COUNT(integers); i++)
    {
        TEST_EXPECT(homie_value_valid(HOMIE_DATATYPE_INTEGER, integers[i]));
    }
    const str not_integers[] = { STR("21.5"), STR("1e3"), STR("+5"), STR(" 5"), STR("-"), STR("5-"), STR("--5"), STR("9223372036854775808"), STR("-9223372036854775809") };
    for (u32 i = 0; i < ARRAY_COUNT(not_integers); i++)
    {
        TEST_EXPECT(!homie_value_valid(HOMIE_DATATYPE_INTEGER, not_integers[i]));
    }
}

void homie_test_value_float(void)
{
    const str floats[] = { STR("21.5"), STR("-0.25"), STR("21"), STR("1e3"), STR("1.5E-3"), STR("-7e-2"), STR("007.50") };
    for (u32 i = 0; i < ARRAY_COUNT(floats); i++)
    {
        TEST_EXPECT(homie_value_valid(HOMIE_DATATYPE_FLOAT, floats[i]));
    }

    const str not_floats[] = { STR(".5"), STR("5."), STR("-"), STR("1e"), STR("1e-"), STR("1e+3"), STR("+1"), STR("1.2.3"), STR("1,5"), STR(" 1"), STR("1 "), STR("NaN"), STR("Infinity"), STR("0x10"), STR_EMPTY };
    for (u32 i = 0; i < ARRAY_COUNT(not_floats); i++)
    {
        TEST_EXPECT(!homie_value_valid(HOMIE_DATATYPE_FLOAT, not_floats[i]));
    }
}

void homie_test_value_json(void)
{
    const str jsons[] = { STR("{}"), STR("[]"), STR("{\"on\":true,\"level\":[1,2.5]}"), STR(" [ 1 ] ") };
    for (u32 i = 0; i < ARRAY_COUNT(jsons); i++)
    {
        TEST_EXPECT(homie_value_valid(HOMIE_DATATYPE_JSON, jsons[i]));
    }

    const str not_jsons[] = { STR("true"), STR("21"), STR("\"text\""), STR("null"), STR("{"), STR("[1,]"),  STR("{} x"), STR_EMPTY };
    for (u32 i = 0; i < ARRAY_COUNT(not_jsons); i++)
    {
        TEST_EXPECT(!homie_value_valid(HOMIE_DATATYPE_JSON, not_jsons[i]));
    }
}