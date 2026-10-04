#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include "base/base.h"

typedef struct JsonWriter JsonWriter;
struct JsonWriter
{
    Writer out;
    b32 should_place_comma;
};

JsonWriter json_writer_create(u8 *buffer, u64 capacity);

void json_write_object_begin(JsonWriter *w, str key);
void json_write_object_end(JsonWriter *w);

void json_write_str(JsonWriter *w, str key, str value);
void json_write_i64(JsonWriter *w, str key, i64 value);
void json_write_bool(JsonWriter *w, str key, b32 value);

#endif