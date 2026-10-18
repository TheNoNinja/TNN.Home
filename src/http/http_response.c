#include "http/http_response.h"

str http_status_reason(const u32 status)
{
    switch (status)
    {
        case 200: return STR("OK");
        case 204: return STR("No Content");
        case 400: return STR("Bad Request");
        case 403: return STR("Forbidden");
        case 404: return STR("Not Found");
        case 405: return STR("Method Not Allowed");
        case 413: return STR("Content Too Large");
        case 418: return STR("I'm a teapot");
        case 422: return STR("Unprocessable Content");
        case 431: return STR("Request Header Fields Too Large");
        case 500: return STR("Internal Server Error");
        case 503: return STR("Service Unavailable");
        default: return STR_EMPTY;
    }
}

void http_write_status(Writer *w, const u32 status)
{
    write_bytes(w, STR("HTTP/1.1 "));
    write_u64_str(w, status);
    write_bytes(w, STR(" "));
    write_bytes(w, http_status_reason(status));
    write_bytes(w, STR("\r\n"));
}

void http_write_header(Writer *w, const str name, const str value)
{
    write_bytes(w, name);
    write_bytes(w, STR(": "));
    write_bytes(w, value);
    write_bytes(w, STR("\r\n"));
}

void http_write_header_u64(Writer *w, const str name, const u64 value)
{
    write_bytes(w, name);
    write_bytes(w, STR(": "));
    write_u64_str(w, value);
    write_bytes(w, STR("\r\n"));
}

void http_write_head_end(Writer *w)
{
    write_bytes(w, STR("\r\n"));
}

void http_write_escaped(Writer *w, const str text)
{
    for (u64 i = 0; i < text.length; i++)
    {
        switch (text.data[i])
        {
            case '&': write_bytes(w, STR("&amp;")); break;
            case '<': write_bytes(w, STR("&lt;")); break;
            case '>': write_bytes(w, STR("&gt;")); break;
            case '"': write_bytes(w, STR("&quot;")); break;
            case '\'': write_bytes(w, STR("&#39;")); break;
            default: write_u8(w, text.data[i]); break;
        }
    }
}

void http_write_event(Writer *w, const str data)
{
    u64 start = 0;
    for (u64 i = 0; i <= data.length; i++)
    {
        const b32 is_end = i == data.length;
        if (!is_end && data.data[i] != '\r' && data.data[i] != '\n')
        {
            continue;
        }
        write_bytes(w, STR("data: "));
        if (i > start)
        {
            write_bytes(w, (str){ data.data + start, i - start });
        }
        write_bytes(w, STR("\n"));
        if (!is_end && data.data[i] == '\r' && i + 1 < data.length && data.data[i + 1] == '\n')
        {
            i += 1;
        }
        start = i + 1;
    }
    write_bytes(w, STR("\n"));
}