#ifndef JSON_READER_H
#define JSON_READER_H

#include "base/base.h"

typedef struct JsonReader JsonReader;
struct JsonReader
{
    const u8 *data;
    u64 length;
    u64 position;
    b32 should_have_comma;
    b32 has_error;
};

JsonReader json_reader_create(const u8 *data, u64 length);
void json_read_object_begin(JsonReader *r);
b32 json_read_key(JsonReader *r, str *key);
str json_read_str(JsonReader *r);
i64 json_read_i64(JsonReader *r);
b32 json_read_bool(JsonReader *r);
void json_read_skip(JsonReader *r);
void json_read_end(JsonReader *r);

#endif