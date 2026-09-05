#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include "base/base.h"

typedef struct JsonWriter JsonWriter;
struct JsonWriter
{
    u8 *data;
    u64 capacity;
    u64 length;
    b32 has_overflown;
    b32 should_place_comma;
};

JsonWriter json_writer_create(u8 *buffer, u64 capacity);

void json_write_object_begin(JsonWriter *w);
void json_write_object_end(JsonWriter *w);
void json_write_key(JsonWriter *w, str key);

void json_write_str(JsonWriter *w, str value);
void json_write_i64(JsonWriter *w, i64 value);
void json_write_bool(JsonWriter *w, b32 value);

#endif