#include "http/http_request.h"

internal b32 http_next_line(str *text, str *line)
{
    u64 end = 0;
    if (!str_find(*text, STR("\r\n"), &end))
    {
        return 0;
    }
    *line = (str){ text->data, end };
    *text = str_skip(*text, end + 2);
    return 1;
}

internal b32 http_token_valid(const str s)
{
    const str signs = STR("!#$%&'*+-.^_`|~");
    for (u64 i = 0; i < s.length; i++)
    {
        const u8 c = s.data[i];
        b32 is_allowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        for (u64 k = 0; k < signs.length && !is_allowed; k++)
        {
            is_allowed = c == signs.data[k];
        }
        if (!is_allowed)
        {
            return 0;
        }
    }
    return s.length > 0;
}

internal b32 http_visible(const str s)
{
    for (u64 i = 0; i < s.length; i++)
    {
        if (s.data[i] < 0x21 || s.data[i] > 0x7E)
        {
            return 0;
        }
    }
    return 1;
}

internal b32 http_value_valid(const str s)
{
    for (u64 i = 0; i < s.length; i++)
    {
        const u8 c = s.data[i];
        if ((c < 0x20 && c != '\t') || c == 0x7F)
        {
            return 0;
        }
    }
    return 1;
}

internal b32 http_name_is(const str name, const str lower)
{
    if (name.length != lower.length)
    {
        return 0;
    }

    for(u64 i = 0; i < name.length; i++)
    {
        u8 c = name.data[i];
        if (c >= 'A' && c <= 'Z')
        {
            c = (u8)(c + ('a' - 'A'));
        }
        if (c != lower.data[i])
        {
            return 0;
        }
    }
    return 1;
}

HttpRead http_request_read(const str bytes, const u64 body_max, HttpRequest *request)
{
    u64 end = 0;
    if (!str_find(bytes, STR("\r\n\r\n"), &end))
    {
        return HTTP_READ_INCOMPLETE;
    }
    const u64 head_length = end + 4;

    str lines = { bytes.data, head_length - 2};
    str line = STR_EMPTY;
    http_next_line(&lines, &line);

    HttpRequest r = {0};
    str rest = STR_EMPTY;
    str version = STR_EMPTY;
    if (!str_cut(line, ' ', &r.method, &rest) || !str_cut(rest, ' ', &r.target, &version))
    {
        return HTTP_READ_BAD;
    }
    const b32 is_target_ok = r.target.length > 0 && r.target.data[0] == '/' && http_visible(r.target);
    if (!http_token_valid(r.method) || !is_target_ok || !str_equal(version, STR("HTTP/1.1")))
    {
        return HTTP_READ_BAD;
    }

    b32 has_host = 0;
    b32 has_length = 0;
    b32 has_origin = 0;
    u64 content_length = 0;
    while(http_next_line(&lines, &line))
    {
        str name = STR_EMPTY;
        str value = STR_EMPTY;
        if (!str_cut(line, ':', &name, &value) || !http_token_valid(name))
        {
            return HTTP_READ_BAD;
        }
        value = str_trim(value);
        if (!http_value_valid(value))
        {
            return HTTP_READ_BAD;
        }
        if (http_name_is(name, STR("host")))
        {
            if (has_host || value.length == 0 || !http_visible(value))
            {
                return HTTP_READ_BAD;
            }
            has_host = 1;
            r.host = value;
        }
        else if ( http_name_is(name, STR("content-length")))
        {
            if (has_length || !str_to_u64(value, &content_length))
            {
                return HTTP_READ_BAD;
            }
            has_length = 1;
        }
        else if (http_name_is(name, STR("transfer-encoding")))
        {
            return HTTP_READ_BAD;
        }
        else if (http_name_is(name, STR("origin")))
        {
            if (has_origin || value.length == 0 || !http_visible(value))
            {
                return HTTP_READ_BAD;
            }
            has_origin = 1;
            r.origin = value;
        }
    }

    if (!has_host)
    {
        return HTTP_READ_BAD;
    }
    if (content_length > body_max)
    {
        return HTTP_READ_TOO_LARGE;
    }
    if (bytes.length - head_length < content_length)
    {
        return HTTP_READ_INCOMPLETE;
    }

    r.head_length = head_length;
    r.body = (str){ bytes.data + head_length, content_length };
    *request = r;
    return HTTP_READ_OK;
}

b32 http_request_host_is(const HttpRequest *request, const str ip, const u16 port)
{
    str name = STR_EMPTY;
    str digits = STR_EMPTY;
    u64 number = 0;
    if (!str_cut(request->host, ':', &name, &digits) || !str_to_u64(digits, &number) || number != port)
    {
        return 0;
    }
    return str_equal(name, ip) || str_equal(name, STR("localhost"));
}

b32 http_request_same_origin(const HttpRequest *request)
{
    const str scheme = STR("http://");
    const str origin = request->origin;
    if (origin.length == 0)
    {
        return 1;
    }
    return str_starts_with(origin, scheme) && str_equal(str_skip(origin, scheme.length), request->host);
}