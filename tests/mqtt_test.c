#include "test.h"
#include "mqtt/mqtt.h"

void mqtt_test_str(void)
{
    const u8 expected[] = { 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x00, 0x00 };

    u8 buffer[16];
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
    mqtt_write_str(&w, STR_LITERAL("MQTT"));
    mqtt_write_str(&w, STR_LITERAL(""));
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));
}

typedef struct MqttTestVarint MqttTestVarint;
struct MqttTestVarint {
    u32 value;
    u8 bytes[4];
    u64 length;
};

void mqtt_test_varint(void)
{
    const MqttTestVarint cases[] = {
        { .value = 0,         .bytes = { 0x00 },                   .length = 1 },
        { .value = 127,       .bytes = { 0x7F },                   .length = 1 },
        { .value = 128,       .bytes = { 0x80, 0x01 },             .length = 2 },
        { .value = 16383,     .bytes = { 0xFF, 0x7F },             .length = 2 },
        { .value = 16384,     .bytes = { 0x80, 0x80, 0x01 },       .length = 3 },
        { .value = 2097151,   .bytes = { 0xFF, 0xFF, 0x7F },       .length = 3 },
        { .value = 2097152,   .bytes = { 0x80, 0x80, 0x80, 0x01 }, .length = 4 },
        { .value = 268435455, .bytes = { 0xFF, 0xFF, 0xFF, 0x7F }, .length = 4 },
    };
    const u32 count = sizeof(cases) / sizeof(cases[0]);

    for (u32 i = 0; i < count; i += 1)
    {
        u8 buffer[8];
        MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
        mqtt_write_varint(&w, cases[i].value);
        TEST_EXPECT_BYTES(w.data, w.length, cases[i].bytes, cases[i].length);
    }
}

void mqtt_test_varint_to_big(void)
{
    u8 buffer[8];
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
    mqtt_write_varint(&w, 268435456);
    TEST_EXPECT(w.overflow);
    TEST_EXPECT(w.length == 0);
}

void mqtt_test_connect(void)
{
    const u8 expected[] = {
        0x10, 0x13,                       //Connect + size
        0x00, 0x04, 'M', 'Q', 'T', 'T',   //Protocol
        0x04,                             //Protocol version
        0x02,                             //Clean session flag
        0x00, 0x00,                       //No keep alive
        0x00, 0x07,                       //Client id length
        't', 'n', 'n', 't', 'e', 's', 't' //Client id
    };

    u8 buffer[64];
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
    mqtt_write_connect(&w, STR_LITERAL("tnntest"), 0);
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));
}