#include "homie/homie_description.h"
#include "json/json.h"

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