#include "test.h"
#include "mqtt/mqtt.h"
#include "mqtt/mqtt_reader.h"

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
struct MqttTestVarint
{
    u32 value;
    u8 bytes[4];
    u64 length;
};
internal const MqttTestVarint mqtt_test_varint_cases[] = {
        { .value = 0,         .bytes = { 0x00 },                   .length = 1 },
        { .value = 127,       .bytes = { 0x7F },                   .length = 1 },
        { .value = 128,       .bytes = { 0x80, 0x01 },             .length = 2 },
        { .value = 16383,     .bytes = { 0xFF, 0x7F },             .length = 2 },
        { .value = 16384,     .bytes = { 0x80, 0x80, 0x01 },       .length = 3 },
        { .value = 2097151,   .bytes = { 0xFF, 0xFF, 0x7F },       .length = 3 },
        { .value = 2097152,   .bytes = { 0x80, 0x80, 0x80, 0x01 }, .length = 4 },
        { .value = 268435455, .bytes = { 0xFF, 0xFF, 0xFF, 0x7F }, .length = 4 },
    };

void mqtt_test_varint(void)
{
    const MqttTestVarint *cases = mqtt_test_varint_cases;
    const u32 count = ARRAY_COUNT(mqtt_test_varint_cases);

    for (u32 i = 0; i < count; i++)
    {
        u8 buffer[8];
        MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
        mqtt_write_varint(&w, cases[i].value);
        TEST_EXPECT_BYTES(w.data, w.length, cases[i].bytes, cases[i].length);
    }
}

void mqtt_test_varint_too_big(void)
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
    mqtt_write_connect(&w, STR_LITERAL("tnntest"), 0, STR_EMPTY, STR_EMPTY);
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));
}

void mqtt_test_connect_will(void)
{
    const u8 expected[] = {
        0x10, 0x23,                            //Connect + size
        0x00, 0x04, 'M', 'Q', 'T', 'T',        //Protocol
        0x04,                                  //Protocol version
        0x26,                                  //Will retain + will + clean session flags
        0x00, 0x0A,                            //Keep alive 10s
        0x00, 0x07,                            //Client id length
        't', 'n', 'n', 't', 'e', 's', 't',     //Client id
        0x00, 0x08,                            //Will topic length
        'w', '/', '$', 's', 't', 'a', 't', 'e',//Will topic
        0x00, 0x04,                            //Will payload length
        'l', 'o', 's', 't'                     //Will payload
    };

    u8 buffer[64];
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
    mqtt_write_connect(&w, STR_LITERAL("tnntest"), 10, STR_LITERAL("w/$state"), STR_LITERAL("lost"));
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));
}

void mqtt_test_pingreq(void) 
{
    const u8 expected [] = { 0xC0, 0x00 };

    u8 buffer[4];
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
    mqtt_write_pingreq(&w);
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));
}

void mqtt_test_read_str(void) 
{
    const u8 packet[] = { 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x00, 0x00 };
    const u8 mqtt[] = { 'M', 'Q', 'T', 'T' };

    MqttReader r = mqtt_reader_create(packet, sizeof(packet));
    const str first = mqtt_read_str(&r);
    const str second = mqtt_read_str(&r);
    TEST_EXPECT_BYTES(first.data, first.length, mqtt, sizeof(mqtt));
    TEST_EXPECT(first.data == packet + 2);
    TEST_EXPECT(second.length == 0);
    TEST_EXPECT(!r.error && r.position == sizeof(packet));
}

void mqtt_test_read_past_end(void)
{
    const u8 packet[] = { 0x12, 0x34 };
    MqttReader r = mqtt_reader_create(packet, sizeof(packet));
    TEST_EXPECT(mqtt_read_u16_be(&r) == 0x1234);
    TEST_EXPECT(!r.error);
    TEST_EXPECT(mqtt_read_u8(&r) == 0);
    TEST_EXPECT(r.error);
    TEST_EXPECT(mqtt_read_u16_be(&r) == 0 && mqtt_read_str(&r).length == 0);
    TEST_EXPECT(r.error && r.position == 2);
}

void mqtt_test_read_varint(void)
{
    const MqttTestVarint *cases = mqtt_test_varint_cases;
    const u32 count = (u32)ARRAY_COUNT(mqtt_test_varint_cases);

    for (u32 i = 0; i < count; i++)
    {
        MqttReader r = mqtt_reader_create(cases[i].bytes, cases[i].length);
        TEST_EXPECT(mqtt_read_varint(&r) == cases[i].value);
        TEST_EXPECT(!r.error && r.position == cases[i].length);
    }
}

void mqtt_test_read_varint_too_long(void) 
{
    const u8 five_bytes[] = { 0x80, 0x80, 0x80, 0x80, 0x01 };
    MqttReader r = mqtt_reader_create(five_bytes, sizeof(five_bytes));
    TEST_EXPECT(mqtt_read_varint(&r) == 0);
    TEST_EXPECT(r.error);

    const u8 cut_off[] = { 0x80 };
    r = mqtt_reader_create(cut_off, sizeof(cut_off));
    TEST_EXPECT(mqtt_read_varint(&r) == 0);
    TEST_EXPECT(r.error);
}