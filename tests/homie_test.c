#include "test.h"
#include "homie/homie.h"
#include "json/json.h"

void homie_test_topic(void)
{
    HomieTopic t = {0};
    TEST_EXPECT(homie_topic_parse(STR("homie/5/tnn-sim-1/$state"), &t));
    TEST_EXPECT(str_equal(t.domain, STR("homie")));
    TEST_EXPECT(str_equal(t.device, STR("tnn-sim-1")));
    TEST_EXPECT(str_equal(t.rest, STR("$state")));

    TEST_EXPECT(homie_topic_parse(STR("homie/5/tnn-sim-1/light/on/set"), &t));
    TEST_EXPECT(str_equal(t.rest, STR("light/on/set")));

    TEST_EXPECT(homie_topic_parse(STR("test-2/5/a/$description"), &t));
    TEST_EXPECT(str_equal(t.domain, STR("test-2")) && str_equal(t.device, STR("a")));
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
    TEST_EXPECT(!homie_topic_parse(STR("tnn/echo-probe"), &t));
    TEST_EXPECT(homie_topic_parse(STR("homie/5/abcdefghijklmnopqrstuvwxyz012345/$state"), &t));
    TEST_EXPECT(!homie_topic_parse(STR("homie/5/abcdefghijklmnopqrstuvwxyz0123456/$state"), &t));
}

void homie_test_topic_build(void)
{
    u8 buffer[64];
    const str topic = homie_topic_build(buffer, sizeof(buffer), STR("homie"), STR("tnn-sim-1"), STR("$description"));
    TEST_EXPECT(str_equal(topic, STR("homie/5/tnn-sim-1/$description")));

    HomieTopic t = {0};
    TEST_EXPECT(homie_topic_parse(topic, &t));
    TEST_EXPECT(str_equal(t.domain, STR("homie")) && str_equal(t.device, STR("tnn-sim-1")) && str_equal(t.rest, STR("$description")));
    TEST_EXPECT(homie_topic_build(buffer, 30, STR("homie"), STR("tnn-sim-1"), STR("$description")).length == 30);
    TEST_EXPECT(homie_topic_build(buffer, 29, STR("homie"), STR("tnn-sim-1"), STR("$description")).length == 0);
}

void homie_test_id(void)
{
    u8 bytes[] = "tnn-sim-1";
    const HomieId id = homie_id_copy((str){ bytes, sizeof(bytes) - 1 });
    bytes[0] = 'X';
    TEST_EXPECT(str_equal(homie_id_str(&id), STR("tnn-sim-1")));
}

void homie_test_state(void)
{
    TEST_EXPECT(homie_state_parse(STR("init")) == HOMIE_STATE_INIT);
    TEST_EXPECT(homie_state_parse(STR("ready")) == HOMIE_STATE_READY);
    TEST_EXPECT(homie_state_parse(STR("lost")) == HOMIE_STATE_LOST);
    TEST_EXPECT(homie_state_parse(STR("Ready")) == HOMIE_STATE_NONE);
    TEST_EXPECT(homie_state_parse(STR_EMPTY) == HOMIE_STATE_NONE);
    TEST_EXPECT(str_equal(homie_state_name(HOMIE_STATE_SLEEPING), STR("sleeping")));
}

void homie_test_datatype(void)
{
    TEST_EXPECT(homie_datatype_parse(STR("boolean")) == HOMIE_DATATYPE_BOOLEAN);
    TEST_EXPECT(homie_datatype_parse(STR("json")) == HOMIE_DATATYPE_JSON);
    TEST_EXPECT(homie_datatype_parse(STR("bool")) == HOMIE_DATATYPE_NONE);
    TEST_EXPECT(homie_datatype_parse(STR("Float")) == HOMIE_DATATYPE_NONE);
    TEST_EXPECT(homie_datatype_parse(STR("none")) == HOMIE_DATATYPE_NONE);
    TEST_EXPECT(str_equal(homie_datatype_name(HOMIE_DATATYPE_DURATION), STR("duration")));
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
    TEST_EXPECT(str_equal(homie_id_str(&on->id), STR("temp")) && temp->datatype == HOMIE_DATATYPE_FLOAT);
    TEST_EXPECT(!on->is_settable && !on->is_retained);
    TEST_EXPECT(str_equal(homie_id_str(&d.properties[1].node), STR("switch")) && d.properties[1].is_settable);

    u8 buffer[1024];
    JsonWriter w = json_writer_create(buffer, sizeof(buffer));
    json_write_object_begin(&w);
    json_write_key(&w, STR("homie"));
    json_write_str(&w, STR("5.0"));
    json_write_key(&w, STR("version"));
    json_write_i64(&w, 1);
    json_write_key(&w, STR("nodes"));
    json_write_object_begin(&w);
    json_write_key(&w, STR("n"));
    json_write_object_begin(&w);
    json_write_key(&w, STR("properties"));
    json_write_object_begin(&w);
    for (u32 i = 0; i < HOMIE_PROPERTY_MAX + 1; i++)
    {
        u8 id[2] = { 'p', (u8)('a' + i) };
        json_write_key(&w, (str){ id, sizeof(id) });
        json_write_object_begin(&w);
        json_write_key(&w, STR("datatype"));
        json_write_str(&w, STR("string"));
        json_write_object_end(&w);
    }
    json_write_object_end(&w);
    json_write_object_end(&w);
    json_write_object_end(&w);
    json_write_object_end(&w);

    TEST_EXPECT(!w.has_overflown);
    TEST_EXPECT(homie_description_read((str) { w.data, w.length }, &d));
    TEST_EXPECT(d.property_count == HOMIE_PROPERTY_MAX);
    TEST_EXPECT(str_equal(homie_id_str(&d.properties[15].id), STR("pp")));
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