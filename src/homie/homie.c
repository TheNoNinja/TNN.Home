#include "homie/homie.h"
#include "json/json.h"

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

internal HomieProperty homie_property_read(JsonReader *r)
{
    HomieProperty p = {0};
    p.is_retained = 1;
    json_read_object_begin(r);
    str key = STR_EMPTY;
    while(json_read_key(r, &key))
    {
        if (str_equal(key, STR("datatype")))
        {
            p.datatype = homie_datatype_parse(json_read_str(r));
        }
        else if (str_equal(key, STR("settable")))
        {
            p.is_settable = json_read_bool(r);
        }
        else if (str_equal(key, STR("retained")))
        {
            p.is_retained = json_read_bool(r);
        } 
        else 
        {
            json_read_skip(r);
        }
    }
    return p;
}

internal void homie_properties_read(JsonReader *r, const str node, HomieDescription *out)
{
    json_read_object_begin(r);
    str id = STR_EMPTY;
    while(json_read_key(r, &id))
    {
        HomieProperty p = homie_property_read(r);
        if (r->has_error)
        {
            return;
        }
        if (!homie_id_valid(id) || p.datatype == HOMIE_DATATYPE_NONE)
        {
            LOG_WARN("$description: property %.*s/%.*s has no valid id or datatype, ignored", STR_FMT(node), STR_FMT(id));
            continue;
        }
        if(out->property_count == HOMIE_PROPERTY_MAX)
        {
            LOG_WARN("$description: property %.*s/%.*s: more than %d properties, ignored", STR_FMT(node), STR_FMT(id), HOMIE_PROPERTY_MAX);
            continue;
        }
        p.node = homie_id_copy(node);
        p.id = homie_id_copy(id);
        out->properties[out->property_count] = p;
        out->property_count += 1;
    }
}

internal void homie_node_read(JsonReader *r, const str node, HomieDescription *out)
{
    json_read_object_begin(r);
    str key = STR_EMPTY;
    while(json_read_key(r, &key))
    {
        if (str_equal(key, STR("properties")))
        {
            homie_properties_read(r, node, out);
        }
        else 
        {
            json_read_skip(r);
        }
    }
}

internal void homie_nodes_read(JsonReader *r, HomieDescription *out)
{
    json_read_object_begin(r);
    str node = STR_EMPTY;
    while(json_read_key(r, &node))
    {
        if (homie_id_valid(node))
        {
            homie_node_read(r, node, out);   
        }
        else 
        {
            LOG_WARN("$description: node %.*s has no valid id, ignored", STR_FMT(node));
            json_read_skip(r);
        }
    }
}

b32 homie_description_parse(const str json, HomieDescription *out)
{
    *out = (HomieDescription){0};
    b32 is_homie_5 = 0;
    b32 has_version = 0;

    JsonReader r = json_reader_create(json.data, json.length);
    json_read_object_begin(&r);
    str key = STR_EMPTY;
    while (json_read_key(&r, &key))
    {
        if (str_equal(key, STR("homie")))
        {
            str major = STR_EMPTY;
            str minor = STR_EMPTY;
            u64 number = 0;
            is_homie_5 = str_cut(json_read_str(&r), '.', &major, &minor) && str_equal(major, STR("5")) && str_to_u64(minor, &number);
        }
        else if (str_equal(key, STR("version")))
        {
            out->version = json_read_i64(&r);
            has_version = 1;
        }
        else if (str_equal(key, STR("nodes")))
        {
            homie_nodes_read(&r, out);
        }
        else
        {
            json_read_skip(&r);
        }
    }
    json_read_end(&r);
    if (r.has_error)
    {
        LOG_WARN("$description: not JSON, or a field or the wrong kind (at byte %llu)", (unsigned long long)r.position);
        return 0;
    }
    if (!is_homie_5 || !has_version)
    {
        LOG_WARN("$description: no \"homie\" 5.x version, or no \"version\" provided");
        return 0;
    }
    return 1;
}

i32 homie_property_find(const HomieDescription *d, const str node, const str property)
{
    for (u32 i = 0; i < d->property_count; i++)
    {
        const HomieProperty *p = &d->properties[i];
        if (str_equal(homie_id_str(&p->node), node) && str_equal(homie_id_str(&p->id), property))
        {
            return (i32)i;
        }
    }
    return -1;
}

internal b32 homie_payload_take(str *s, const u8 c)
{
    if (s->length > 0 && s->data[0] == c)
    {
        *s = str_skip(*s, 1);
        return 1;
    }
    return 0;
}

internal b32 homie_payload_integer(const str value)
{
    i64 number = 0;
    return str_to_i64(value, &number);
}

internal u64 homie_payload_take_digits(str *s)
{
    u64 count = 0;
    while (count < s->length && s->data[count] >= '0' && s->data[count] <= '9')
    {
        count += 1;
    }
    *s = str_skip(*s, count);
    return count;
}

internal b32 homie_payload_float(const str value)
{
    str rest = value;
    homie_payload_take(&rest, '-');
    if (homie_payload_take_digits(&rest) == 0)
    {
        return 0;
    }
    if (homie_payload_take(&rest, '.') && homie_payload_take_digits(&rest) == 0)
    {
        return 0;
    }
    if (homie_payload_take(&rest, 'e') || homie_payload_take(&rest, 'E'))
    {
        homie_payload_take(&rest, '-');
        if (homie_payload_take_digits(&rest) == 0)
        {
            return 0;
        }
    }
    return rest.length == 0;
}

internal b32 homie_payload_json(const str value)
{
    JsonReader r = json_reader_create(value.data, value.length);
    json_read_skip(&r);
    if (r.has_error)
    {
        return 0;
    }
    const u8 last = r.data[r.position - 1];
    json_read_end(&r);
    return !r.has_error && (last == '}' || last == ']');
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

b32 homie_payload_valid(const HomieDatatype datatype, const str value)
{
    if (value.length == 0)
    {
        return 0;
    }
    
    switch (datatype)
    {
        case HOMIE_DATATYPE_NONE:     return 0;
        case HOMIE_DATATYPE_BOOLEAN:  return str_equal(value, STR("true")) || str_equal(value, STR("false"));
        case HOMIE_DATATYPE_INTEGER:  return homie_payload_integer(value);
        case HOMIE_DATATYPE_FLOAT:    return homie_payload_float(value);
        case HOMIE_DATATYPE_JSON:     return homie_payload_json(value);
        case HOMIE_DATATYPE_STRING:
        case HOMIE_DATATYPE_ENUM:
        case HOMIE_DATATYPE_COLOR:
        case HOMIE_DATATYPE_DATETIME:
        case HOMIE_DATATYPE_DURATION: return 1;
    }
    
    return 0;
}