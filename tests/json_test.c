#include "test.h"
#include "json/json.h"

void json_test_object(void)
{
    const str expected = STR_LITERAL("{\"name\":\"light\",\"on\":true,\"off\":false,\"inner\":{\"empty\":{},\"min\":-9223372036854775808,\"max\":9223372036854775807},\"zero\":0}");
    u8 buffer[256];
    JsonWriter w = json_writer_create(buffer, sizeof(buffer));
    json_write_object_begin(&w);
    json_write_key(&w, STR_LITERAL("name"));
    json_write_str(&w, STR_LITERAL("light"));
    json_write_key(&w, STR_LITERAL("on"));
    json_write_bool(&w, 1);
    json_write_key(&w, STR_LITERAL("off"));
    json_write_bool(&w, 0);
    json_write_key(&w, STR_LITERAL("inner"));
    json_write_object_begin(&w);
    json_write_key(&w, STR_LITERAL("empty"));
    json_write_object_begin(&w);
    json_write_object_end(&w);
    json_write_key(&w, STR_LITERAL("min"));
    json_write_i64(&w, INT64_MIN);
    json_write_key(&w, STR_LITERAL("max"));
    json_write_i64(&w, INT64_MAX);
    json_write_object_end(&w);
    json_write_key(&w, STR_LITERAL("zero"));
    json_write_i64(&w, 0);
    json_write_object_end(&w);
    TEST_EXPECT(!w.overflow);
    TEST_EXPECT_BYTES(w.data, w.length, expected.data, expected.length);
}