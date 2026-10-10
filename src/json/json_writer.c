#include "json/json_writer.h"

JsonWriter json_writer_create(u8 *buffer, const u64 capacity)
{
    JsonWriter w = {0};
    w.out = writer_create(buffer, capacity);
    return w;
}

internal void json_write_comma(JsonWriter *w)
{
    if (w->should_place_comma)
    {
        write_u8(&w->out, ',');
    }
}

internal void json_write_quoted(JsonWriter *w, const str s)
{
    write_u8(&w->out, '"');
    for (u64 i = 0; i < s.length; i++)
    {
        const u8 c = s.data[i];
        if (c == '"' || c == '\\')
        {
            write_u8(&w->out, '\\');
            write_u8(&w->out, c);
        }
        else if (c < 0x20)
        {
            const char *hex = "0123456789abcdef";
            write_bytes(&w->out, STR("\\u00"));
            write_u8(&w->out, (u8)hex[c >> 4]);
            write_u8(&w->out, (u8)hex[c & 0x0F]);
        }
        else
        {
            write_u8(&w->out, c);
        }
    }
    write_u8(&w->out, '"');
}

internal void json_write_key(JsonWriter *w, const str key)
{
    if (str_equal(key, STR_EMPTY))
    {
        w->should_place_comma = 0;
        return;
    }
    json_write_quoted(w, key);
    write_u8(&w->out, ':');
    w->should_place_comma = 0;
}

void json_write_object_begin(JsonWriter *w, const str key)
{
    json_write_comma(w);
    json_write_key(w, key);
    write_u8(&w->out, '{');
    w->should_place_comma = 0;
}

void json_write_object_end(JsonWriter *w)
{
    write_u8(&w->out, '}');
    w->should_place_comma = 1;
}

void json_write_str(JsonWriter *w, const str key, const str value)
{
    json_write_comma(w);
    json_write_key(w, key);
    json_write_quoted(w, value);
    w->should_place_comma = 1;
}

void json_write_i64(JsonWriter *w, const str key, const i64 value)
{
    json_write_comma(w);
    json_write_key(w, key);
    write_i64_str(&w->out, value);
    w->should_place_comma = 1;
}

void json_write_bool(JsonWriter *w, const str key, const b32 value)
{
    json_write_comma(w);
    json_write_key(w, key);
    write_bytes(&w->out, value ? STR("true") : STR("false"));
    w->should_place_comma = 1;
}