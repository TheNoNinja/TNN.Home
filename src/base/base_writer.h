#ifndef BASE_WRITER_H
#define BASE_WRITER_H

#include "base/base_str.h"

typedef struct Writer Writer;
struct Writer
{
    u8 *data;
    u64 capacity;
    u64 length;
    b32 has_overflown;
};

Writer writer_create(u8 *buffer, u64 capacity);
str writer_to_str(const Writer *w);

void write_u8(Writer *w, u8 value);
void write_bytes(Writer *w, str bytes);
void write_u16_be(Writer *w, u16 value);
void write_u64_be(Writer *w, u64 value);
void write_u64_str(Writer *w, u64 value);
void write_i64_str(Writer *w, i64 value);

#endif