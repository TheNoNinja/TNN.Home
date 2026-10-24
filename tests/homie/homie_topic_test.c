#include "test.h"
#include "homie/homie.h"

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