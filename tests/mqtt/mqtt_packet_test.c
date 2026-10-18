#include "test.h"
#include "mqtt/mqtt.h"

void mqtt_test_str(void)
{
    const u8 expected[] = { 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x00, 0x00 };

    u8 buffer[16];
    Writer w = writer_create(buffer, sizeof(buffer));
    mqtt_write_str(&w, STR("MQTT"));
    mqtt_write_str(&w, STR(""));
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
        Writer w = writer_create(buffer, sizeof(buffer));
        mqtt_write_varint(&w, cases[i].value);
        TEST_EXPECT_BYTES(w.data, w.length, cases[i].bytes, cases[i].length);
    }
}

void mqtt_test_varint_too_big(void)
{
    u8 buffer[8];
    Writer w = writer_create(buffer, sizeof(buffer));
    mqtt_write_varint(&w, 268435456);
    TEST_EXPECT(w.has_overflown);
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
    Writer w = writer_create(buffer, sizeof(buffer));
    mqtt_write_connect(&w, STR("tnntest"), 0, STR_EMPTY, STR_EMPTY);
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
    Writer w = writer_create(buffer, sizeof(buffer));
    mqtt_write_connect(&w, STR("tnntest"), 10, STR("w/$state"), STR("lost"));
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));
}

void mqtt_test_subscribe(void) 
{
    const u8 expected[] = {
        0x82, 0x0E,                                  // Subscribe + size
        0x00, 0x01,                                  // Packet id 1
        0x00, 0x09,                                  // Topic filter length
        'h', 'o', 'm', 'i', 'e', '/', '5', '/', '#', // Topic filter
        0x00                                         // QoS 0
    };

    u8 buffer[32];
    Writer w = writer_create(buffer, sizeof(buffer));
    mqtt_write_subscribe(&w, 1, STR("homie/5/#"));
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));
}

void mqtt_test_unsubscribe(void)
{
    const u8 expected[] = {
        0xA2, 0x0D,                                 // Unsubscribe + size
        0x00, 0x02,                                 // Packet id 2
        0x00, 0x09,                                 // Topic filter length
        'h', 'o', 'm', 'i', 'e', '/', '5', '/', '#' // Topic filter
    };

    u8 buffer[32];
    Writer w = writer_create(buffer, sizeof(buffer));
    mqtt_write_unsubscribe(&w, 2, STR("homie/5/#"));
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));
}

void mqtt_test_pingreq(void) 
{
    const u8 expected [] = { 0xC0, 0x00 };

    u8 buffer[4];
    Writer w = writer_create(buffer, sizeof(buffer));
    mqtt_write_pingreq(&w);
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));
}

void mqtt_test_publish(void)
{
    const u8 expected[] = {0x30, 0x05, 0x00, 0x03, 'a', '/', 'b' };
    u8 buffer[16];
    Writer w = writer_create(buffer, sizeof(buffer));
    mqtt_write_publish(&w, STR("a/b"), (str){0}, 0);
    TEST_EXPECT(!w.has_overflown);
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));
}

void mqtt_test_read_str(void) 
{
    const u8 packet[] = { 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x00, 0x00 };
    const u8 mqtt[] = { 'M', 'Q', 'T', 'T' };

    Reader r = reader_create(packet, sizeof(packet));
    const str first = mqtt_read_str(&r);
    const str second = mqtt_read_str(&r);
    TEST_EXPECT_BYTES(first.data, first.length, mqtt, sizeof(mqtt));
    TEST_EXPECT(first.data == packet + 2);
    TEST_EXPECT(second.length == 0);
    TEST_EXPECT(!r.has_error && r.position == sizeof(packet));
}

void mqtt_test_read_past_end(void)
{
    const u8 packet[] = { 0x12, 0x34 };
    Reader r = reader_create(packet, sizeof(packet));
    TEST_EXPECT(read_u16_be(&r) == 0x1234);
    TEST_EXPECT(!r.has_error);
    TEST_EXPECT(read_u8(&r) == 0);
    TEST_EXPECT(r.has_error);
    TEST_EXPECT(read_u16_be(&r) == 0 && mqtt_read_str(&r).length == 0);
    TEST_EXPECT(r.has_error && r.position == 2);
}

void mqtt_test_read_varint(void)
{
    const MqttTestVarint *cases = mqtt_test_varint_cases;
    const u32 count = (u32)ARRAY_COUNT(mqtt_test_varint_cases);

    for (u32 i = 0; i < count; i++)
    {
        Reader r = reader_create(cases[i].bytes, cases[i].length);
        TEST_EXPECT(mqtt_read_varint(&r) == cases[i].value);
        TEST_EXPECT(!r.has_error && r.position == cases[i].length);
    }
}

void mqtt_test_read_varint_too_long(void) 
{
    const u8 five_bytes[] = { 0x80, 0x80, 0x80, 0x80, 0x01 };
    Reader r = reader_create(five_bytes, sizeof(five_bytes));
    TEST_EXPECT(mqtt_read_varint(&r) == 0);
    TEST_EXPECT(r.has_error);

    const u8 cut_off[] = { 0x80 };
    r = reader_create(cut_off, sizeof(cut_off));
    TEST_EXPECT(mqtt_read_varint(&r) == 0);
    TEST_EXPECT(r.has_error);
}

void mqtt_test_read_fixed_header(void) 
{
    const u8 packet[] = { 0x31, 0x82, 0x01, 0xD0, 0x00 };
    Reader r = reader_create(packet, sizeof(packet));

    MqttFixedHeader header = mqtt_read_fixed_header(&r);
    TEST_EXPECT(header.type == MQTT_PUBLISH && header.flags == MQTT_PUBLISH_FLAG_RETAIN && header.remaining_length == 130);
    header = mqtt_read_fixed_header(&r);
    TEST_EXPECT(header.type == MQTT_PINGRESP && header.flags == 0 && header.remaining_length == 0);
    TEST_EXPECT(!r.has_error && r.position == sizeof(packet));
}

void mqtt_test_read_publish(void) 
{
    const u8 packet[] = { 0x00, 0x03, 'a', '/', 'b', 'o', 'n' };
    const u8 topic[] = { 'a', '/', 'b' };
    const u8 payload[] = { 'o', 'n' };

    Reader r = reader_create(packet, sizeof(packet));
    MqttPublish m = mqtt_read_publish(&r, MQTT_PUBLISH_FLAG_RETAIN);
    TEST_EXPECT_BYTES(m.topic.data, m.topic.length, topic, sizeof(topic));
    TEST_EXPECT_BYTES(m.payload.data, m.payload.length, payload, sizeof(payload));
    TEST_EXPECT(m.is_retained && m.qos == 0);
    TEST_EXPECT(!r.has_error && r.position == sizeof(packet));

    const u8 packet_qos1[] = { 0x00, 0x03, 'a', '/', 'b', 0x12, 0x34, 'o', 'n' };
    r = reader_create(packet_qos1, sizeof(packet_qos1));
    m = mqtt_read_publish(&r, 0x02);
    TEST_EXPECT_BYTES(m.payload.data, m.payload.length, payload, sizeof(payload));
    TEST_EXPECT(!m.is_retained && m.qos == 1 && !r.has_error);

    const u8 liar_packet[] = { 0x00, 0x09, 'a', '/', 'b' };
    r = reader_create(liar_packet, sizeof(liar_packet));
    m = mqtt_read_publish(&r, 0);
    TEST_EXPECT(r.has_error && m.topic.length == 0 && m.payload.length == 0);
}

void mqtt_test_packet_parse(void)
{
    MqttPacket p = {0};
    u8 accepted[] = { 0x00, 0x00 };
    u8 refused[]  = { 0x00, 0x05 };
    const MqttFixedHeader connack = { .type = MQTT_CONNACK, .remaining_length = 2 };
    TEST_EXPECT(mqtt_packet_parse(connack, (str){ accepted, 2 }, &p) && p.type == MQTT_CONNACK && p.return_code == 0);
    TEST_EXPECT(mqtt_packet_parse(connack, (str){ refused, 2 }, &p) && p.return_code == 5);

    u8 suback[]       = { 0x00, 0x01, MQTT_SUBACK_QOS_0 };
    u8 suback_no[]    = { 0x00, 0x01, MQTT_SUBACK_REFUSED };
    const MqttFixedHeader subacks = { .type = MQTT_SUBACK, .remaining_length = 3 };
    TEST_EXPECT(mqtt_packet_parse(subacks, (str){ suback, 3 }, &p) && p.packet_id == 1 && p.return_code == 0);
    TEST_EXPECT(mqtt_packet_parse(subacks, (str){ suback_no, 3 }, &p) && p.return_code == MQTT_SUBACK_REFUSED);

    u8 zeros[] = { 0x00, 0x00, 0x00 };
    const MqttFixedHeader long_connack = { .type = MQTT_CONNACK, .remaining_length = 3 };
    const MqttFixedHeader short_suback = { .type = MQTT_SUBACK, .remaining_length = 2 };
    const MqttFixedHeader short_unsuback = { .type = MQTT_UNSUBACK, .remaining_length = 1 };
    TEST_EXPECT(!mqtt_packet_parse(long_connack, (str){ zeros, 3 }, &p));
    TEST_EXPECT(!mqtt_packet_parse(short_suback, (str){ zeros, 2 }, &p));
    TEST_EXPECT(!mqtt_packet_parse(short_unsuback, (str){ zeros, 1 }, &p));

    const MqttFixedHeader puback = { .type = 4, .remaining_length = 2 };
    TEST_EXPECT(mqtt_packet_parse(puback, (str){ zeros, 2 }, &p) && p.type == 4 && p.packet_id == 0);
}