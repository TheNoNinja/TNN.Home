#include "test.h"
#include "mqtt/mqtt.h"

#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

internal void mqtt_test_connection_feed(MqttFramer *in, const u8 *bytes, const u64 count)
{
    const str space = mqtt_framer_space(in);
    ASSERT(count <= space.length);
    memcpy(space.data, bytes, count);
    mqtt_framer_received(in, count);
}

internal u64 mqtt_test_connection_sent(const int end, u8 *bytes, const u64 capacity)
{
    const ssize_t count = recv(end, bytes, capacity, MSG_DONTWAIT);
    return count > 0 ? (u64)count : 0;
}

void mqtt_test_connection_send(void)
{
    int ends[2];
    TEST_EXPECT(socketpair(AF_UNIX, SOCK_STREAM, 0, ends) == 0);
    u8 in[16];
    u8 out[16];
    MqttConnection c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    c.sock = ends[0];
    u8 got[32];

    const u8 publish[] = { 0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n' };
    TEST_EXPECT(mqtt_connection_publish(&c, STR("a/b"), STR("on"), 1));
    u64 length = mqtt_test_connection_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, publish, sizeof(publish));

    const u8 subscribe[] = { 0x82, 0x08, 0x00, 0x02, 0x00, 0x03, 'a', '/', '#', 0x00 };
    TEST_EXPECT(mqtt_connection_subscribe(&c, STR("a/b")) == 1);
    mqtt_test_connection_sent(ends[1], got, sizeof(got));
    TEST_EXPECT(mqtt_connection_subscribe(&c, STR("a/#")) == 2);
    length = mqtt_test_connection_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, subscribe, sizeof(subscribe));

    const u8 pingreq[] = { 0xC0, 0x00 };
    TEST_EXPECT(mqtt_connection_ping(&c) && c.is_waiting_on_ping);
    length = mqtt_test_connection_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, pingreq, sizeof(pingreq));

    TEST_EXPECT(!mqtt_connection_ping(&c));
    TEST_EXPECT(mqtt_test_connection_sent(ends[1], got, sizeof(got)) == 0);

    TEST_EXPECT(!mqtt_connection_publish(&c, STR("a/b"), STR("something way to long to fit in 16 bytes"), 0));
    TEST_EXPECT(mqtt_test_connection_sent(ends[1], got, sizeof(got)) == 0);

    const u8 disconnect[] = { 0xE0, 0x00 };
    TEST_EXPECT(mqtt_connection_disconnect(&c) && c.sock == -1);
    length = mqtt_test_connection_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, disconnect, sizeof(disconnect));
    close(ends[1]);
}

void mqtt_test_connection_next(void)
{
    const u8 bytes[] = {
        0x90, 0x03, 0x00, 0x01, 0x00,                   //SUBACK
        0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n',//PUBLISH
        0xD0, 0x00,                                     //PINGRESP
        0x40, 0x02, 0x00, 0x07                          //PUBACK
    };

    u8 in[32];
    u8 out[8];
    MqttConnection c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    mqtt_test_connection_feed(&c.in, bytes, sizeof(bytes));
    c.is_waiting_on_ping = 1;
    MqttPacket p = {0};
    TEST_EXPECT(mqtt_connection_next(&c, &p) && p.type == MQTT_SUBACK && p.packet_id == 1);
    TEST_EXPECT(mqtt_connection_next(&c, &p) && p.type == MQTT_PUBLISH && p.publish.is_retained);
    TEST_EXPECT(str_equal(p.publish.topic, STR("a/b")) && str_equal(p.publish.payload, STR("on")));
    TEST_EXPECT(mqtt_connection_next(&c, &p) && p.type == MQTT_PINGRESP && !c.is_waiting_on_ping);
    TEST_EXPECT(!mqtt_connection_next(&c, &p) && !c.has_error);
}

void mqtt_test_connection_refused(void)
{
    const u8 refused[] = { 0x90, 0x03, 0x00, 0x01, 0x80 };
    u8 in[16];
    u8 out[8];
    MqttConnection c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    MqttPacket p = {0};
    mqtt_test_connection_feed(&c.in, refused, sizeof(refused));
    TEST_EXPECT(!mqtt_connection_next(&c, &p) && c.has_error);

    const u8 liar[] = { 0x30, 0x05, 0x00, 0x09, 'a', '/', 'b' };
    c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    mqtt_test_connection_feed(&c.in, liar, sizeof(liar));
    TEST_EXPECT(!mqtt_connection_next(&c, &p) && c.has_error);
}

void mqtt_test_connection_unsubscribe(void)
{
    int ends[2];
    TEST_EXPECT(socketpair(AF_UNIX, SOCK_STREAM, 0, ends) == 0);
    u8 in[16];
    u8 out[16];
    MqttConnection c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    c.sock = ends[0];
    u8 got[32];

    const u8 unsubscribe[] = { 0xA2, 0x07, 0x00, 0x02, 0x00, 0x03, 'a', '/', '#' };
    TEST_EXPECT(mqtt_connection_subscribe(&c, STR("a/#")) == 1);
    mqtt_test_connection_sent(ends[1], got, sizeof(got));
    TEST_EXPECT(mqtt_connection_unsubscribe(&c, STR("a/#")) == 2);
    const u64 length = mqtt_test_connection_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, unsubscribe, sizeof(unsubscribe));

    const u8 unsuback[] = { 0xB0, 0x02, 0x00, 0x02 };
    mqtt_test_connection_feed(&c.in, unsuback, sizeof(unsuback));
    MqttPacket p = {0};
    TEST_EXPECT(mqtt_connection_next(&c, &p) && p.type == MQTT_UNSUBACK && p.packet_id == 2);
    mqtt_connection_close(&c);
    close(ends[1]);
}