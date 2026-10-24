#include "test.h"
#include "homie/homie.h"

#include <string.h>

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