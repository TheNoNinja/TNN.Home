#include "homie/homie.h"

#include <string.h>

b32 homie_id_valid(const str id)
{
    if (id.length == 0 || id.length > HOMIE_ID_MAX)
    {
        return 0;
    }
    for (u64 i = 0; i < id.length; i++)
    {
        const u8 c = id.data[i];
        const b32 allowed = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
        if (!allowed)
        {
            return 0;
        }
    }
    return 1;
}

HomieId homie_id_copy(const str id)
{
    ASSERT(homie_id_valid(id));
    HomieId copy = {0};
    memcpy(copy.data, id.data, id.length);
    copy.length = (u8)id.length;
    return copy;
}

str homie_id_str(const HomieId *id)
{
    return (str){ .data = (u8*)id->data, .length = id->length };
}

b32 homie_topic_parse(const str topic, HomieTopic *out)
{
    str version = STR_EMPTY;
    str rest = STR_EMPTY;
    const b32 cut = str_cut(topic, '/', &out->domain, &rest) && str_cut(rest, '/', &version, &rest) && str_cut(rest, '/', &out->device, &out->rest); 
    return cut && homie_id_valid(out->domain) && str_equal(version, STR("5")) && homie_id_valid(out->device) && out->rest.length > 0;
}

internal u8 *homie_topic_put(u8 *at, const str s)
{
    if (s.length > 0)
    {
        memcpy(at, s.data, s.length);
    }
    return at + s.length;
}

str homie_topic_build(u8 *buffer, const u64 capacity, const str domain, const str device, const str rest)
{
    const u64 length = domain.length + 3 + device.length + 1 + rest.length;
    if (length > capacity)
    {
        return STR_EMPTY;
    }
    u8 *at = homie_topic_put(buffer, domain);
    at = homie_topic_put(at, STR("/5/"));
    at = homie_topic_put(at, device);
    at = homie_topic_put(at, STR("/"));
    homie_topic_put(at, rest);
    return (str){ .data = buffer, .length = length };
}

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