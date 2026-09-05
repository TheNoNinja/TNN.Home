#include "json/json_reader.h"

#define JSON_READER_DEPTH_MAX 16

JsonReader json_reader_create(const u8 *data, const u64 length)
{
    JsonReader r = {0};
    r.data = data;
    r.length = length;
    return r;
}

internal u8 json_peek(JsonReader *r)
{
    while (!r->has_error && r->position < r->length)
    {
        const u8 c = r->data[r->position];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
        {
            return c;
        }
        r->position += 1;
    }
    return 0;
}

internal void json_expect(JsonReader *r, const u8 c)
{
    if (json_peek(r) != c)
    {
        r->has_error = 1;
        return;
    }
    r->position += 1;
}

internal b32 json_hex(const u8 c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

internal void json_read_escape(JsonReader *r)
{
    if (r->position >= r->length)
    {
        r->has_error = 1;
        return;
    }

    const u8 c = r->data[r->position];
    r->position += 1;
    if (c == 'u')
    {
        for (u32 i = 0; i < 4; i++)
        {
            if (r->position >= r->length || !json_hex(r->data[r->position]))
            {
                r->has_error = 1;
                return;
            }
            r->position += 1;
        }
        return;
    }
    const b32 is_known_escape = c == '"' || c == '\\' || c == '/' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't';
    if (!is_known_escape)
    {
        r->has_error = 1;
    }
}

void json_read_object_begin(JsonReader *r)
{
    json_expect(r, '{');
    r->should_have_comma = 0;
}

b32 json_read_key(JsonReader *r, str *key)
{
    if (json_peek(r) == '}')
    {
        r->position += 1;
        r->should_have_comma = 1;
        return 0;
    }
    if (r->should_have_comma)
    {
        json_expect(r, ',');
    }
    *key = json_read_str(r);
    json_expect(r, ':');
    r->should_have_comma = 0;
    return !r->has_error;
}

str json_read_str(JsonReader *r)
{
    json_expect(r, '"');
    const u64 start = r->position;
    while (!r->has_error && r->position < r->length)
    {
        const u8 c = r->data[r->position];
        r->position += 1;
        if (c == '"')
        {
            r->should_have_comma = 1;
            return (str) { .data = (u8*) r->data + start, .length = r->position - 1 - start };
        }
        if (c == '\\')
        {
            json_read_escape(r);
        }
        else if (c < 0x20)
        {
            r->has_error = 1;
        }
    }
    r->has_error = 1;
    return STR_EMPTY;
}

internal b32 json_take(JsonReader *r, const u8 c)
{
    if (!r->has_error && r->position < r->length && r->data[r->position] == c)
    {
        r->position += 1;
        return 1;
    }
    return 0;
}

internal u64 json_take_digits(JsonReader *r)
{
    const u64 start = r->position;
    while (!r->has_error && r->position < r->length && r->data[r->position] >= '0' && r->data[r->position] <= '9')
    {
        r->position += 1;
    }
    return r->position - start;
}

internal str json_read_number(JsonReader *r)
{
    json_peek(r);
    const u64 start = r->position;
    json_take(r, '-');
    const u64 first = r->position;
    const u64 digits = json_take_digits(r);
    if (digits == 0 || (digits > 1 && r->data[first] == '0'))
    {
        r->has_error = 1;
    }
    if (json_take(r, '.') && json_take_digits(r) == 0)
    {
        r->has_error = 1;
    }
    if (json_take(r, 'e') || json_take(r, 'E'))
    {
        if (!json_take(r, '+'))
        {
            json_take(r, '-');
        }
        if (json_take_digits(r) == 0)
        {
            r->has_error = 1;
        }
    }
    r->should_have_comma = 1;
    return (str) { .data = (u8*)(r->data + start), .length = r->position - start };
}

i64 json_read_i64(JsonReader *r)
{
    const str text = json_read_number(r);
    const b32 negative = text.length > 0 && text.data[0] == '-';
    const str digits = negative ? (str){ .data = text.data + 1, .length = text.length - 1 } : text;
    const u64 limit = negative ? (u64)INT64_MAX + 1 : (u64)INT64_MAX;
    u64 magnitude = 0;
    if (r->has_error || !str_to_u64(digits, &magnitude) || magnitude > limit)
    {
        r->has_error = 1;
        return 0;
    }
    if (!negative || magnitude == 0)
    {
        return (i64)magnitude;
    }
    return -(i64)(magnitude - 1) - 1;
}

internal void json_read_word(JsonReader *r, const str word)
{
    json_peek(r);
    const str next = { .data = (u8 *)(r->data + r->position), .length = r->length - r->position };
    if (r->has_error || next.length < word.length || !str_equal((str) { .data = next.data, .length = word.length }, word))
    {
        r->has_error = 1;
        return;
    }

    r->position += word.length;
    r->should_have_comma = 1;
}

b32 json_read_bool(JsonReader *r)
{
    if (json_peek(r) == 't')
    {
        json_read_word(r, STR("true"));
        return !r->has_error;
    }
    json_read_word(r, STR("false"));
    return 0;
}

internal void json_skip(JsonReader *r, const u32 depth)
{
    if (depth >= JSON_READER_DEPTH_MAX)
    {
        r->has_error = 1;
        return;
    }
    switch (json_peek(r))
    {
        case '{':
        {
            json_read_object_begin(r);
            str key = STR_EMPTY;
            while (json_read_key(r, &key))
            {
                json_skip(r, depth + 1);
            }
            break;
        }
        case '[':
        {
            r->position += 1;
            r->should_have_comma = 0;
            while (json_peek(r) != ']' && !r->has_error)
            {
                if (r->should_have_comma)
                {
                    json_expect(r, ',');
                }
                json_skip(r, depth + 1);
            }
            json_expect(r, ']');
            r->should_have_comma = 1;
            break;
        }
        case '"': json_read_str(r); break;
        case 't':
        case 'f': json_read_bool(r); break;
        case 'n': json_read_word(r, STR("null")); break;
        default: json_read_number(r); break;
    }
}

void json_read_skip(JsonReader *r)
{
    json_skip(r, 0);
}

void json_read_end(JsonReader *r)
{
    json_peek(r);
    if (r->position != r->length)
    {
        r->has_error = 1;
    }
}