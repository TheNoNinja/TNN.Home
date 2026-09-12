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

b32 homie_description_read(const str json, HomieDescription *out)
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