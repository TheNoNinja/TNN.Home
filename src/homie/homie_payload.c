#include "homie/homie_payload.h"

#include <string.h>

str homie_state_name(const HomieState state)
{
    switch(state)
    {
    case HOMIE_STATE_NONE:         return STR("none");
    case HOMIE_STATE_INIT:         return STR("init");
    case HOMIE_STATE_READY:        return STR("ready");
    case HOMIE_STATE_DISCONNECTED: return STR("disconnected");
    case HOMIE_STATE_SLEEPING:     return STR("sleeping");
    case HOMIE_STATE_LOST:         return STR("lost");
    }
    return STR("none");
}

HomieState homie_state_parse(const str payload)
{
    for (u32 s = HOMIE_STATE_INIT; s <= HOMIE_STATE_LOST; s++)
    {
        if (str_equal(payload, homie_state_name((HomieState)s)))
        {
            return (HomieState)s;
        }
    }
    return HOMIE_STATE_NONE;
}

str homie_datatype_name(const HomieDatatype datatype)
{
    switch (datatype)
    {
    case HOMIE_DATATYPE_NONE: return STR("none");
    case HOMIE_DATATYPE_INTEGER: return STR("integer");
    case HOMIE_DATATYPE_FLOAT: return STR("float");
    case HOMIE_DATATYPE_BOOLEAN: return STR("boolean");
    case HOMIE_DATATYPE_STRING: return STR("string");
    case HOMIE_DATATYPE_ENUM: return STR("enum");
    case HOMIE_DATATYPE_COLOR: return STR("color");
    case HOMIE_DATATYPE_DATETIME: return STR("datetime");
    case HOMIE_DATATYPE_DURATION: return STR("duration");
    case HOMIE_DATATYPE_JSON: return STR("json");
    }
    return STR("none");
}

HomieDatatype homie_datatype_parse(const str name)
{
    for (u32 d = HOMIE_DATATYPE_INTEGER; d <= HOMIE_DATATYPE_JSON; d++)
    {
        if (str_equal(name, homie_datatype_name(d)))
        {
            return (HomieDatatype)d;
        }
    }
    return HOMIE_DATATYPE_NONE;
}

b32 homie_value_set(HomieValue *v, const str value)
{
    if (value.length > HOMIE_VALUE_MAX)
    {
        *v = (HomieValue){0};
        return 0;
    }
    if (value.length > 0)
    {
        memcpy(v->bytes, value.data, value.length);
    }
    v->length = value.length;
    v->is_known = value.length > 0;
    return 1;
}

str homie_value_str(const HomieValue *v)
{
    return v->is_known ? (str){ (u8 *)v->bytes, v->length } : STR_EMPTY;
}