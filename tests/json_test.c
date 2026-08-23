#include "test.h"
#include "json/json.h"

void json_test_object(void)
{
    const str expected = STR("{\"name\":\"light\",\"on\":true,\"off\":false,\"inner\":{\"empty\":{},\"min\":-9223372036854775808,\"max\":9223372036854775807},\"zero\":0}");
    u8 buffer[256];
    JsonWriter w = json_writer_create(buffer, sizeof(buffer));
    json_write_object_begin(&w);
    json_write_key(&w, STR("name"));
    json_write_str(&w, STR("light"));
    json_write_key(&w, STR("on"));
    json_write_bool(&w, 1);
    json_write_key(&w, STR("off"));
    json_write_bool(&w, 0);
    json_write_key(&w, STR("inner"));
    json_write_object_begin(&w);
    json_write_key(&w, STR("empty"));
    json_write_object_begin(&w);
    json_write_object_end(&w);
    json_write_key(&w, STR("min"));
    json_write_i64(&w, INT64_MIN);
    json_write_key(&w, STR("max"));
    json_write_i64(&w, INT64_MAX);
    json_write_object_end(&w);
    json_write_key(&w, STR("zero"));
    json_write_i64(&w, 0);
    json_write_object_end(&w);
    TEST_EXPECT(!w.overflow);
    TEST_EXPECT_BYTES(w.data, w.length, expected.data, expected.length);
}

void json_test_escape(void)
{
    const str expected = STR("\"a\\\"b\\\\c\\u000a\\u0001\xc3\xa9\"");
    u8 buffer[64];
    JsonWriter w = json_writer_create(buffer, sizeof(buffer));
    json_write_str(&w, STR("a\"b\\c\n\x01" "\xc3\xa9"));
    TEST_EXPECT(!w.overflow);
    TEST_EXPECT_BYTES(w.data, w.length, expected.data, expected.length);
}

void json_test_overflow(void)
{
    u8 buffer[12];
    for (u32 i = 0; i < sizeof(buffer); i++)
    {
        buffer[i] = 0xAA;
    }
    JsonWriter w = json_writer_create(buffer, 8);
    json_write_object_begin(&w);
    json_write_key(&w, STR("settable"));
    json_write_bool(&w, 1);
    json_write_object_end(&w);
    TEST_EXPECT(w.overflow);
    TEST_EXPECT(w.length <= 8);
    TEST_EXPECT(buffer[8] == 0xAA && buffer[9] == 0xAA && buffer[10] == 0xAA && buffer[11] == 0xAA);
}