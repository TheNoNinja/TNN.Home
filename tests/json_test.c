#include "test.h"
#include "json/json.h"

#include <stdio.h>

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
    TEST_EXPECT(!w.has_overflown);
    TEST_EXPECT_BYTES(w.data, w.length, expected.data, expected.length);
}

void json_test_escape(void)
{
    const str expected = STR("\"a\\\"b\\\\c\\u000a\\u0001\xc3\xa9\"");
    u8 buffer[64];
    JsonWriter w = json_writer_create(buffer, sizeof(buffer));
    json_write_str(&w, STR("a\"b\\c\n\x01" "\xc3\xa9"));
    TEST_EXPECT(!w.has_overflown);
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
    TEST_EXPECT(w.has_overflown);
    TEST_EXPECT(w.length <= 8);
    TEST_EXPECT(buffer[8] == 0xAA && buffer[9] == 0xAA && buffer[10] == 0xAA && buffer[11] == 0xAA);
}

void json_test_read(void)
{
    const str text = STR(" {\"name\" : \"TNN \\\"light\\\"\",\n \"inner\":{\"empty\":{}, \"e\":\"\\u00e9\"} }\n");
    JsonReader r = json_reader_create(text.data, text.length);
    str key = STR_EMPTY;

    json_read_object_begin(&r);
    TEST_EXPECT(json_read_key(&r, &key) && str_equal(key, STR("name")));
    const str name = json_read_str(&r);
    TEST_EXPECT(str_equal(name, STR("TNN \\\"light\\\"")));
    TEST_EXPECT(name.data == text.data + 12);
    TEST_EXPECT(json_read_key(&r, &key) && str_equal(key, STR("inner")));
    json_read_object_begin(&r);
    TEST_EXPECT(json_read_key(&r, &key) && str_equal(key, STR("empty")));
    json_read_object_begin(&r);
    TEST_EXPECT(!json_read_key(&r, &key));
    TEST_EXPECT(json_read_key(&r, &key) && str_equal(key, STR("e")));
    TEST_EXPECT(str_equal(json_read_str(&r), STR("\\u00e9")));
    TEST_EXPECT(!json_read_key(&r, &key));
    TEST_EXPECT(!json_read_key(&r, &key));
    json_read_end(&r);
    TEST_EXPECT(!r.has_error);

    const str broken = STR("{\"name\" \"light\"}");
    r = json_reader_create(broken.data, broken.length);
    json_read_object_begin(&r);
    TEST_EXPECT(!json_read_key(&r, &key) && r.has_error);
    TEST_EXPECT(json_read_str(&r).length == 0 && r.has_error);
}

void json_test_read_numbers(void)
{
    const str text = STR("{\"min\":-9223372036854775808,\"max\":9223372036854775807,\"zero\":-0}");
    JsonReader r = json_reader_create(text.data, text.length);
    str key = STR_EMPTY;
    json_read_object_begin(&r);
    TEST_EXPECT(json_read_key(&r, &key) && json_read_i64(&r) == INT64_MIN);
    TEST_EXPECT(json_read_key(&r, &key) && json_read_i64(&r) == INT64_MAX);
    TEST_EXPECT(json_read_key(&r, &key) && json_read_i64(&r) == 0);
    TEST_EXPECT(!json_read_key(&r, &key));
    json_read_end(&r);
    TEST_EXPECT(!r.has_error);

    const str numbers[] = { STR("9223372036854775808"), STR("-9223372036854775809"), STR("1.5"), STR("1e3"), STR("07"), STR("-"), STR("+1"), STR("1.") };
    for (u32 i = 0; i < ARRAY_COUNT(numbers); i++)
    {
        r = json_reader_create(numbers[i].data, numbers[i].length);
        json_read_i64(&r);
        TEST_EXPECT(r.has_error);
    }
}

void json_test_read_booleans(void)
{
    const str text = STR("{\"on\":true,\"off\":false}");
    JsonReader r = json_reader_create(text.data, text.length);
    str key = STR_EMPTY;
    json_read_object_begin(&r);
    TEST_EXPECT(json_read_key(&r, &key) && json_read_bool(&r));
    TEST_EXPECT(json_read_key(&r, &key) && !json_read_bool(&r));
    TEST_EXPECT(!json_read_key(&r, &key));
    json_read_end(&r);
    TEST_EXPECT(!r.has_error);

    const str words[] = { STR("True"), STR("fals"), STR("null") };
    for (u32 i = 0; i < ARRAY_COUNT(words); i++)
    {
        r = json_reader_create(words[i].data, words[i].length);
        json_read_bool(&r);
        TEST_EXPECT(r.has_error);
    }
}

void json_test_read_skip(void)
{
    const str text = STR("{\"a\":[1.5e3,-0.25,null,true,\"x\",{\"b\":[]},[[]]],\"c\":{\"d\":{}},\"last\":42}");
    JsonReader r = json_reader_create(text.data, text.length);
    str key = STR_EMPTY;
    json_read_object_begin(&r);
    TEST_EXPECT(json_read_key(&r, &key) && str_equal(key, STR("a")));
    json_read_skip(&r);
    TEST_EXPECT(json_read_key(&r, &key) && str_equal(key, STR("c")));
    json_read_skip(&r);
    TEST_EXPECT(json_read_key(&r, &key) && str_equal(key, STR("last")) && json_read_i64(&r) == 42);
    TEST_EXPECT(!json_read_key(&r, &key));
    json_read_end(&r);
    TEST_EXPECT(!r.has_error);

    u8 deep[34];
    for (u32 i = 0; i < 17; i++)
    {
        deep[i] = '[';
        deep[33-i] = ']';
    }
    r = json_reader_create(deep + 1, 32);
    json_read_skip(&r);
    json_read_end(&r);
    TEST_EXPECT(!r.has_error);
    r = json_reader_create(deep, 34);
    json_read_skip(&r);
    TEST_EXPECT(r.has_error);
}

void json_test_read_invalid(void)
{
    const str broken[] = { STR(""), STR("{"), STR("{\"a\":1,}"), STR("{\"a\" 1}"), STR("{a:1}"), STR("[1 2]"), STR("[1,]"), STR("\"abc"), STR("\"\\x\""), STR("\"\\u12g4\""), STR("\"a\nb\""), STR("{\"a\":1} x"), STR("nul") };
    JsonReader r;
    for (u32 i = 0; i < ARRAY_COUNT(broken); i++)
    {
        r = json_reader_create(broken[i].data, broken[i].length);
        json_read_skip(&r);
        json_read_end(&r);
        TEST_EXPECT(r.has_error);
    }
}