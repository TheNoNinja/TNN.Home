#include "test.h"
#include "mqtt/mqtt.h"
#include "mqtt/mqtt_reader.h"

#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void mqtt_test_str(void)
{
    const u8 expected[] = { 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x00, 0x00 };

    u8 buffer[16];
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
    mqtt_write_str(&w, STR("MQTT"));
    mqtt_write_str(&w, STR(""));
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));
}

void mqtt_test_u64(void)
{
    const u8 expected[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    u8 buffer[8];
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
    mqtt_write_u64_be(&w, 0x0102030405060708);
    TEST_EXPECT_BYTES(w.data, w.length, expected, sizeof(expected));

    MqttReader r = mqtt_reader_create(buffer, sizeof(buffer));
    TEST_EXPECT(mqtt_read_u64_be(&r) == 0x0102030405060708 && !r.has_error);
    TEST_EXPECT(mqtt_read_u64_be(&r) == 0 && r.has_error);

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
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
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
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
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
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
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
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
    mqtt_write_unsubscribe(&w, 2, STR("homie/5/#"));
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
    TEST_EXPECT(!r.has_error && r.position == sizeof(packet));
}

void mqtt_test_read_past_end(void)
{
    const u8 packet[] = { 0x12, 0x34 };
    MqttReader r = mqtt_reader_create(packet, sizeof(packet));
    TEST_EXPECT(mqtt_read_u16_be(&r) == 0x1234);
    TEST_EXPECT(!r.has_error);
    TEST_EXPECT(mqtt_read_u8(&r) == 0);
    TEST_EXPECT(r.has_error);
    TEST_EXPECT(mqtt_read_u16_be(&r) == 0 && mqtt_read_str(&r).length == 0);
    TEST_EXPECT(r.has_error && r.position == 2);
}

void mqtt_test_read_varint(void)
{
    const MqttTestVarint *cases = mqtt_test_varint_cases;
    const u32 count = (u32)ARRAY_COUNT(mqtt_test_varint_cases);

    for (u32 i = 0; i < count; i++)
    {
        MqttReader r = mqtt_reader_create(cases[i].bytes, cases[i].length);
        TEST_EXPECT(mqtt_read_varint(&r) == cases[i].value);
        TEST_EXPECT(!r.has_error && r.position == cases[i].length);
    }
}

void mqtt_test_read_varint_too_long(void) 
{
    const u8 five_bytes[] = { 0x80, 0x80, 0x80, 0x80, 0x01 };
    MqttReader r = mqtt_reader_create(five_bytes, sizeof(five_bytes));
    TEST_EXPECT(mqtt_read_varint(&r) == 0);
    TEST_EXPECT(r.has_error);

    const u8 cut_off[] = { 0x80 };
    r = mqtt_reader_create(cut_off, sizeof(cut_off));
    TEST_EXPECT(mqtt_read_varint(&r) == 0);
    TEST_EXPECT(r.has_error);
}

void mqtt_test_read_fixed_header(void) 
{
    const u8 packet[] = { 0x31, 0x82, 0x01, 0xD0, 0x00 };
    MqttReader r = mqtt_reader_create(packet, sizeof(packet));

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

    MqttReader r = mqtt_reader_create(packet, sizeof(packet));
    MqttPublish m = mqtt_read_publish(&r, MQTT_PUBLISH_FLAG_RETAIN);
    TEST_EXPECT_BYTES(m.topic.data, m.topic.length, topic, sizeof(topic));
    TEST_EXPECT_BYTES(m.payload.data, m.payload.length, payload, sizeof(payload));
    TEST_EXPECT(m.is_retained && m.qos == 0);
    TEST_EXPECT(!r.has_error && r.position == sizeof(packet));

    const u8 packet_qos1[] = { 0x00, 0x03, 'a', '/', 'b', 0x12, 0x34, 'o', 'n' };
    r = mqtt_reader_create(packet_qos1, sizeof(packet_qos1));
    m = mqtt_read_publish(&r, 0x02);
    TEST_EXPECT_BYTES(m.payload.data, m.payload.length, payload, sizeof(payload));
    TEST_EXPECT(!m.is_retained && m.qos == 1 && !r.has_error);

    const u8 liar_packet[] = { 0x00, 0x09, 'a', '/', 'b' };
    r = mqtt_reader_create(liar_packet, sizeof(liar_packet));
    m = mqtt_read_publish(&r, 0);
    TEST_EXPECT(r.has_error && m.topic.length == 0 && m.payload.length == 0);
}

internal void mqtt_test_stream_feed(MqttStream *s, const u8 *bytes, const u64 count)
{
    mqtt_stream_compact(s);
    memcpy(s->data + s->length, bytes, count);
    s->length += count;
}

void mqtt_test_stream_split(void) 
{
    const u8 packet[] = { 0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n' };
    u8 buffer[16];
    MqttStream s = mqtt_stream_create(buffer, sizeof(buffer), sizeof(buffer));
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;

    for (u64 i = 0; i + 1 < sizeof(packet); i++)
    {
        mqtt_test_stream_feed(&s, packet + i, 1);
        TEST_EXPECT(!mqtt_stream_next(&s, &header, &body));
    }

    mqtt_test_stream_feed(&s, packet + sizeof(packet) - 1, 1);
    TEST_EXPECT(mqtt_stream_next(&s, &header, &body));
    TEST_EXPECT(header.type == MQTT_PUBLISH && header.flags == MQTT_PUBLISH_FLAG_RETAIN);
    TEST_EXPECT_BYTES(body.data, body.length, packet + 2, 7);
    TEST_EXPECT(!mqtt_stream_next(&s, &header, &body) && !s.has_error);
}

void mqtt_test_stream_burst(void) 
{
    const u8 first[] = {
         0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n',//PUBLISH
         0xD0, 0x00,                                     //PINGRESP
         0x90, 0x03, 0x00                                //Partial SUBACK 
    };
    const u8 last[] = { 0x01, 0x00 };                    //Leftover SUBACK
    const u8 expected[] = { 0x00, 0x01, 0x00 };

    u8 buffer[16];
    MqttStream s = mqtt_stream_create(buffer, sizeof(buffer), sizeof(buffer));
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;

    mqtt_test_stream_feed(&s, first, sizeof(first));
    TEST_EXPECT(mqtt_stream_next(&s, &header, &body) && header.type == MQTT_PUBLISH && body.length == 7);
    TEST_EXPECT(mqtt_stream_next(&s, &header, &body) && header.type == MQTT_PINGRESP && body.length == 0);
    TEST_EXPECT(!mqtt_stream_next(&s, &header, &body));

    mqtt_test_stream_feed(&s, last, sizeof(last));
    TEST_EXPECT(s.position == 0 && s.length == 5);
    TEST_EXPECT(mqtt_stream_next(&s, &header, &body) && header.type == MQTT_SUBACK);
    TEST_EXPECT_BYTES(body.data, body.length, expected, sizeof(expected));
    TEST_EXPECT(!mqtt_stream_next(&s, &header, &body) && !s.has_error);
}

void mqtt_test_stream_oversized(void)
{
    const u8 big[] = { 0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n' };
    const u8 after[] = { 0xD0, 0x00 };

    u8 buffer[8];
    MqttStream s = mqtt_stream_create(buffer, sizeof(buffer), sizeof(buffer));
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;

    mqtt_test_stream_feed(&s, big, 5);
    TEST_EXPECT(!mqtt_stream_next(&s, &header, &body));
    TEST_EXPECT(s.skipped == 1 && s.skip == 4 && s.position == 5);
    mqtt_test_stream_feed(&s, big + 5, 4);
    mqtt_test_stream_feed(&s, after, sizeof(after));
    TEST_EXPECT(mqtt_stream_next(&s, &header, &body) && header.type == MQTT_PINGRESP);
    TEST_EXPECT(!mqtt_stream_next(&s, &header, &body) && !s.has_error && s.skipped == 1 && s.skip == 0);
}

void mqtt_test_stream_malformed(void)
{
    const u8 bad[] = { 0x30, 0x80, 0x80, 0x80, 0x80, 0x01 };
    u8 buffer[16];
    MqttStream s = mqtt_stream_create(buffer, sizeof(buffer), sizeof(buffer));
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;

    mqtt_test_stream_feed(&s, bad, 4);
    TEST_EXPECT(!mqtt_stream_next(&s, &header, &body) && !s.has_error);
    mqtt_test_stream_feed(&s, bad + 4, 2);
    TEST_EXPECT(!mqtt_stream_next(&s, &header, &body));
    TEST_EXPECT(s.has_error && !mqtt_stream_next(&s, &header, &body));
}

internal u64 mqtt_test_client_sent(const int end, u8 *bytes, const u64 capacity)
{
    const ssize_t count = recv(end, bytes, capacity, MSG_DONTWAIT);
    return count > 0 ? (u64)count : 0;
}

void mqtt_test_client_send(void)
{
    int ends[2];
    TEST_EXPECT(socketpair(AF_UNIX, SOCK_STREAM, 0, ends) == 0);
    u8 in[16];
    u8 out[16];
    MqttClient c = mqtt_client_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    c.sock = ends[0];
    u8 got[32];

    const u8 publish[] = { 0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n' };
    TEST_EXPECT(mqtt_client_publish(&c, STR("a/b"), STR("on"), 1));
    u64 length = mqtt_test_client_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, publish, sizeof(publish));

    const u8 subscribe[] = { 0x82, 0x08, 0x00, 0x02, 0x00, 0x03, 'a', '/', '#', 0x00 };
    TEST_EXPECT(mqtt_client_subscribe(&c, STR("a/b")) == 1);
    mqtt_test_client_sent(ends[1], got, sizeof(got));
    TEST_EXPECT(mqtt_client_subscribe(&c, STR("a/#")) == 2);
    length = mqtt_test_client_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, subscribe, sizeof(subscribe));

    const u8 pingreq[] = { 0xC0, 0x00 };
    TEST_EXPECT(mqtt_client_ping(&c) && c.is_waiting_on_ping);
    length = mqtt_test_client_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, pingreq, sizeof(pingreq));

    TEST_EXPECT(!mqtt_client_ping(&c));
    TEST_EXPECT(mqtt_test_client_sent(ends[1], got, sizeof(got)) == 0);

    TEST_EXPECT(!mqtt_client_publish(&c, STR("a/b"), STR("something way to long to fit in 16 bytes"), 0));
    TEST_EXPECT(mqtt_test_client_sent(ends[1], got, sizeof(got)) == 0);

    const u8 disconnect[] = { 0xE0, 0x00 };
    TEST_EXPECT(mqtt_client_disconnect(&c) && c.sock == -1);
    length = mqtt_test_client_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, disconnect, sizeof(disconnect));
    close(ends[1]);
}

void mqtt_test_client_next(void)
{
    const u8 bytes[] = {
        0x90, 0x03, 0x00, 0x01, 0x00,                   //SUBACK
        0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n',//PUBLISH
        0xD0, 0x00,                                     //PINGRESP
        0x40, 0x02, 0x00, 0x07                          //PUBACK
    };

    u8 in[32];
    u8 out[8];
    MqttClient c = mqtt_client_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    mqtt_test_stream_feed(&c.in, bytes, sizeof(bytes));
    c.is_waiting_on_ping = 1;
    MqttPacket p = {0};
    TEST_EXPECT(mqtt_client_next(&c, &p) && p.type == MQTT_SUBACK && p.packet_id == 1);
    TEST_EXPECT(mqtt_client_next(&c, &p) && p.type == MQTT_PUBLISH && p.publish.is_retained);
    TEST_EXPECT(str_equal(p.publish.topic, STR("a/b")) && str_equal(p.publish.payload, STR("on")));
    TEST_EXPECT(mqtt_client_next(&c, &p) && p.type == MQTT_PINGRESP && !c.is_waiting_on_ping);
    TEST_EXPECT(!mqtt_client_next(&c, &p) && !c.has_error);
}

void mqtt_test_client_refused(void)
{
    const u8 refused[] = { 0x90, 0x03, 0x00, 0x01, 0x80 };
    u8 in[16];
    u8 out[8];
    MqttClient c = mqtt_client_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    MqttPacket p = {0};
    mqtt_test_stream_feed(&c.in, refused, sizeof(refused));
    TEST_EXPECT(!mqtt_client_next(&c, &p) && c.has_error);

    const u8 liar[] = { 0x30, 0x05, 0x00, 0x09, 'a', '/', 'b' };
    c = mqtt_client_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    mqtt_test_stream_feed(&c.in, liar, sizeof(liar));
    TEST_EXPECT(!mqtt_client_next(&c, &p) && c.has_error);
}

void mqtt_test_client_unsubscribe(void)
{
    int ends[2];
    TEST_EXPECT(socketpair(AF_UNIX, SOCK_STREAM, 0, ends) == 0);
    u8 in[16];
    u8 out[16];
    MqttClient c = mqtt_client_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    c.sock = ends[0];
    u8 got[32];

    const u8 unsubscribe[] = { 0xA2, 0x07, 0x00, 0x02, 0x00, 0x03, 'a', '/', '#' };
    TEST_EXPECT(mqtt_client_subscribe(&c, STR("a/#")) == 1);
    mqtt_test_client_sent(ends[1], got, sizeof(got));
    TEST_EXPECT(mqtt_client_unsubscribe(&c, STR("a/#")) == 2);
    const u64 length = mqtt_test_client_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, unsubscribe, sizeof(unsubscribe));

    const u8 unsuback[] = { 0xB0, 0x02, 0x00, 0x02 };
    mqtt_test_stream_feed(&c.in, unsuback, sizeof(unsuback));
    MqttPacket p = {0};
    TEST_EXPECT(mqtt_client_next(&c, &p) && p.type == MQTT_UNSUBACK && p.packet_id == 2);
    mqtt_client_close(&c);
    close(ends[1]);
}