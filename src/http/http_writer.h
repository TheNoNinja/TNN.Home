#ifndef HTTP_WRITER_H
#define HTTP_WRITER_H

#include "base/base.h"

typedef struct HttpWriter HttpWriter;
struct HttpWriter
{
    u8 *data;
    u64 capacity;
    u64 length;
    b32 has_overflown;
};

HttpWriter http_writer_create(u8 *buffer, u64 capacity);
str http_writer_str(const HttpWriter *w);

void http_write_str(HttpWriter *w, str text);
void http_write_u64(HttpWriter *w, u64 value);

void http_write_status(HttpWriter *w, u32 status);
void http_write_header(HttpWriter *w, str name, str value);
void http_write_header_u64(HttpWriter *w, str name, u64 value);
void http_write_head_end(HttpWriter *w);
void http_write_escaped(HttpWriter *w, str text);

str http_status_reason(u32 status);
#endif