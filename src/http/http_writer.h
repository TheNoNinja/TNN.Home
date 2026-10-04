#ifndef HTTP_WRITER_H
#define HTTP_WRITER_H

#include "base/base.h"

void write_bytes(Writer *w, str text);
void write_u64_text(Writer *w, u64 value);

void http_write_status(Writer *w, u32 status);
void http_write_header(Writer *w, str name, str value);
void http_write_header_u64(Writer *w, str name, u64 value);
void http_write_head_end(Writer *w);
void http_write_escaped(Writer *w, str text);
void http_write_event(Writer *w, str data);

str http_status_reason(u32 status);
#endif