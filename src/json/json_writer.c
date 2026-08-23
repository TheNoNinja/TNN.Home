#include "json/json_writer.h"

#include <string.h>

JsonWriter json_writer_create(u8 *buffer, const u64 capacity)
{
    ASSERT(buffer != 0);

    JsonWriter w = {0};
    w.data = buffer;
    w.capacity = capacity;
    return w;
}

internal void json_write_char(JsonWriter *w, const u8 c)
{
    if (w->length + 1 > w->capacity)
    {
        w->overflow = 1;
        return;
    }
    w->data[w->length] = c;
    w->length += 1;
}

internal void json_write_bytes(JsonWriter *w, const str bytes)
{
    if (w->length + bytes.length > w->capacity)
    {
        w->overflow = 1;
        return;
    }
    memcpy(w->data + w->length, bytes.data, bytes.length);
    w->length += bytes.length;
}

internal void json_write_comma(JsonWriter *w)
{
    if (w->comma)
    {
        json_write_char(w, ',');
    }
}

void json_write_object_begin(JsonWriter *w)
{
    json_write_comma(w);
    json_write_char(w, '{');
    w->comma = 0;
}

void json_write_object_end(JsonWriter *w)
{
    json_write_char(w, '}');
    w->comma = 1;
}

void json_write_key(JsonWriter *w, const str key)
{
    json_write_str(w, key);
    json_write_char(w, ':');
    w->comma = 0;
}

void json_write_str(JsonWriter *w, const str value)
{
    json_write_comma(w);
    json_write_char(w, '"');
    for (u64 i = 0; i < value.length; i++)
    {
        const u8 c = value.data[i];
        if (c == '"' || c == '\\')
        {
            json_write_char(w, '\\');
            json_write_char(w, c);
        }
        else if (c < 0x20)
        {
            const char *hex = "0123456789abcdef";
            json_write_bytes(w, STR("\\u00"));
            json_write_char(w, (u8)hex[c >> 4]);
            json_write_char(w, (u8)hex[c & 0x0F]);
        }
        else
        {
            json_write_char(w, c);
        }
    }
    json_write_char(w, '"');
    w->comma = 1;
}

void json_write_i64(JsonWriter *w, const i64 value)
{
    json_write_comma(w);
    u64 magnitude = value < 0 ? 0 - (u64)value : (u64)value;
    u8 digits[20];
    u32 count = 0;
    do
    {
        digits[sizeof(digits) - 1 - count] = (u8)('0' + magnitude % 10);
        magnitude = magnitude / 10;
        count += 1;
    } while (magnitude > 0);
    
    if (value < 0)
    {
        json_write_char(w, '-');
    }
    json_write_bytes(w, (str){digits + sizeof(digits) - count, count});
    w->comma = 1;
}

void json_write_bool(JsonWriter *w, const b32 value)
{
    json_write_comma(w);
    json_write_bytes(w, value ? STR("true") : STR("false"));
    w->comma = 1;
}