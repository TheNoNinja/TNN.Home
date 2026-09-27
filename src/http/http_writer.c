#include "http/http_writer.h"

#include <string.h>

HttpWriter http_writer_create(u8 *buffer, const u64 capacity)
{
    ASSERT(buffer != 0);
    HttpWriter w = {0};
    w.data = buffer;
    w.capacity = capacity;
    return w;
}

str http_writer_str(const HttpWriter *w)
{
    return (str){ w->data, w->length };
}

void http_write_str(HttpWriter *w, const str text)
{
    if (w->length + text.length > w->capacity)
    {
        w->has_overflown = 1;
        return;
    }
    if (text.length > 0)
    {
        memcpy(w->data + w->length, text.data, text.length);
    }
    w->length += text.length;
}

void http_write_u64(HttpWriter *w, const u64 value)
{
    u8 digits[20];
    u32 count = 0;
    u64 left = value;
    do
    {
        digits[sizeof(digits) - 1 - count] = (u8)('0' + left % 10);
        left = left / 10;
        count += 1;
    } while (left > 0);
    http_write_str(w, (str){ digits + sizeof(digits) - count, count });
}

str http_status_reason(const u32 status)
{
    switch (status)
    {
        case 200: return STR("OK");
        case 400: return STR("Bad Request");
        case 404: return STR("Not Found");
        case 405: return STR("Method Not Allowed");
        case 418: return STR("I'm a teapot");
        case 431: return STR("Request Header Fields Too Large");
        case 500: return STR("Internal Server Error");
        default: return STR_EMPTY;
    }
}

void http_write_status(HttpWriter *w, const u32 status)
{
    http_write_str(w, STR("HTTP/1.1 "));
    http_write_u64(w, status);
    http_write_str(w, STR(" "));
    http_write_str(w, http_status_reason(status));
    http_write_str(w, STR("\r\n"));
}

void http_write_header(HttpWriter *w, const str name, const str value)
{
    http_write_str(w, name);
    http_write_str(w, STR(": "));
    http_write_str(w, value);
    http_write_str(w, STR("\r\n"));
}

void http_write_header_u64(HttpWriter *w, const str name, const u64 value)
{
    http_write_str(w, name);
    http_write_str(w, STR(": "));
    http_write_u64(w, value);
    http_write_str(w, STR("\r\n"));
}

void http_write_head_end(HttpWriter *w)
{
    http_write_str(w, STR("\r\n"));
}

void http_write_escaped(HttpWriter *w, const str text)
{
    for (u64 i = 0; i < text.length; i++)
    {
        switch (text.data[i])
        {
            case '&': http_write_str(w, STR("&amp;")); break;
            case '<': http_write_str(w, STR("&lt;")); break;
            case '>': http_write_str(w, STR("&gt;")); break;
            case '"': http_write_str(w, STR("&quot;")); break;
            case '\'': http_write_str(w, STR("&#39;")); break;
            default: http_write_str(w, (str){ text.data + i, 1 }); break;
        }
    }
}