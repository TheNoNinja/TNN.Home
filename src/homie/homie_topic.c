#include "homie/homie_topic.h"

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
    str levels[7];
    u32 count = 0;
    str rest = topic;
    b32 has_more = 1;

    while(has_more && count < ARRAY_COUNT(levels))
    {
        has_more = str_cut(rest, '/', &levels[count], &rest);
        count += 1;
    }
    if (count < 4 || count > 6)
    {
        return 0;
    }

    HomieTopic t = {0};
    t.domain = levels[0];
    t.device = levels[2];
    if (!homie_id_valid(t.domain) || !str_equal(levels[1], STR("5")) || !homie_id_valid(t.device))
    {
        return 0;
    }

    const str first = levels[3];
    if (count == 4 && str_equal(first, STR("$state")))
    {
        t.kind = HOMIE_TOPIC_STATE;
    }
    else if (count == 4 && str_equal(first, STR("$description")))
    {
        t.kind = HOMIE_TOPIC_DESCRIPTION;
    }
    else if (count == 5 && str_equal(first, STR("$log")) && homie_id_valid(levels[4]))
    {
        t.kind = HOMIE_TOPIC_LOG;
        t.name = levels[4];
    }
    else if (count == 5 && str_equal(first, STR("$alert")) && homie_id_valid(levels[4]))
    {
        t.kind = HOMIE_TOPIC_ALERT;
        t.name = levels[4];
    }
    else if (count >= 5 && homie_id_valid(first) && homie_id_valid(levels[4]))
    {
        t.node = first;
        t.property = levels[4];
        if (count == 5)
        {
            t.kind = HOMIE_TOPIC_VALUE;
        }
        else if (str_equal(levels[5], STR("$target")))
        {
            t.kind = HOMIE_TOPIC_TARGET;
        }
        else if (str_equal(levels[5], STR("set")))
        {
            t.kind = HOMIE_TOPIC_SET;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
    *out = t;
    return 1;
}

str homie_topic_build(u8 *buffer, const u64 capacity, const HomieTopic *t)
{
    Writer w = writer_create(buffer, capacity);
    write_bytes(&w, t->domain);
    write_bytes(&w, STR("/5/"));
    write_bytes(&w, t->device);
    write_u8(&w, '/');
    switch (t->kind)
    {
        case HOMIE_TOPIC_STATE:       write_bytes(&w, STR("$state")); break;
        case HOMIE_TOPIC_DESCRIPTION: write_bytes(&w, STR("$description")); break;
        case HOMIE_TOPIC_LOG:         write_bytes(&w, STR("$log/")); write_bytes(&w, t->name); break;
        case HOMIE_TOPIC_ALERT:       write_bytes(&w, STR("$alert/")); write_bytes(&w, t->name); break;
        case HOMIE_TOPIC_VALUE:
        case HOMIE_TOPIC_TARGET:
        case HOMIE_TOPIC_SET:
        {
            write_bytes(&w, t->node);
            write_u8(&w, '/');
            write_bytes(&w, t->property);
            if (t->kind == HOMIE_TOPIC_TARGET)
            {
                write_bytes(&w, STR("/$target"));
            }
            if (t->kind == HOMIE_TOPIC_SET)
            {
                write_bytes(&w, STR("/set"));
            }
            break;
        }
    }
    return w.has_overflown ? STR_EMPTY : writer_to_str(&w);
}