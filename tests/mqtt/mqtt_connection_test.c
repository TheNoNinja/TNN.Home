#include "test.h"
#include "mqtt/mqtt.h"

#include <string.h>

internal void mqtt_test_connection_feed(MqttFramer *in, const u8 *bytes, const u64 count)
{
    const str space = mqtt_framer_space(in);
    ASSERT(count <= space.length);
    memcpy(space.data, bytes, count);
    mqtt_framer_received(in, count);
}

internal str mqtt_test_connection_take(MqttConnection *c)
{
    const str out = mqtt_connection_output(c);
    mqtt_connection_sent(c, out.length);
    return out;
}

void mqtt_test_connection_send(void)
{
    u8 in[16];
    u8 out[16];
    MqttConnection c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    c.state = MQTT_CONNECTION_CONNECTED;

    const u8 publish[] = { 0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n' };
    TEST_EXPECT(mqtt_connection_publish(&c, STR("a/b"), STR("on"), 1));
    str sent = mqtt_test_connection_take(&c);
    TEST_EXPECT_BYTES(sent.data, sent.length, publish, sizeof(publish));

    const u8 subscribe[] = { 0x82, 0x08, 0x00, 0x02, 0x00, 0x03, 'a', '/', '#', 0x00 };
    TEST_EXPECT(mqtt_connection_subscribe(&c, STR("a/b")) == 1);
    mqtt_test_connection_take(&c);
    TEST_EXPECT(mqtt_connection_subscribe(&c, STR("a/#")) == 2);
    sent = mqtt_test_connection_take(&c);
    TEST_EXPECT_BYTES(sent.data, sent.length, subscribe, sizeof(subscribe));

    const u8 pingreq[] = { 0xC0, 0x00 };
    TEST_EXPECT(mqtt_connection_ping(&c) && c.is_waiting_on_ping);
    sent = mqtt_test_connection_take(&c);
    TEST_EXPECT_BYTES(sent.data, sent.length, pingreq, sizeof(pingreq));

    TEST_EXPECT(!mqtt_connection_ping(&c));
    TEST_EXPECT(mqtt_connection_output(&c).length == 0);

    TEST_EXPECT(!mqtt_connection_publish(&c, STR("a/b"), STR("something way to long to fit in 16 bytes"), 0));
    TEST_EXPECT(mqtt_connection_output(&c).length == 0);

    const u8 disconnect[] = { 0xE0, 0x00 };
    TEST_EXPECT(mqtt_connection_disconnect(&c) && c.state == MQTT_CONNECTION_DISCONNECTED);
    sent = mqtt_test_connection_take(&c);
    TEST_EXPECT_BYTES(sent.data, sent.length, disconnect, sizeof(disconnect));
}

void mqtt_test_connection_queue(void)
{
    u8 in[16];
    u8 out[16];
    MqttConnection c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    c.state = MQTT_CONNECTION_CONNECTED;
    const u8 both[] = {
        0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n', // PUBLISH
        0xC0, 0x00                                       // PINGREQ
    };

    TEST_EXPECT(mqtt_connection_publish(&c, STR("a/b"), STR("on"), 1) && mqtt_connection_ping(&c));
    str queue = mqtt_connection_output(&c);
    TEST_EXPECT_BYTES(queue.data, queue.length, both, sizeof(both));

    mqtt_connection_sent(&c, 4);
    queue = mqtt_connection_output(&c);
    TEST_EXPECT_BYTES(queue.data, queue.length, both + 4, sizeof(both) - 4);

    TEST_EXPECT(!mqtt_connection_publish(&c, STR("a/b"), STR("off"), 1));
    TEST_EXPECT(mqtt_connection_output(&c).length == sizeof(both) - 4);

    mqtt_connection_sent(&c, sizeof(both) - 4);
    TEST_EXPECT(mqtt_connection_output(&c).length == 0);
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
    c.state = MQTT_CONNECTION_CONNECTED;
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
    c.state = MQTT_CONNECTION_CONNECTED;
    MqttPacket p = {0};
    mqtt_test_connection_feed(&c.in, refused, sizeof(refused));
    TEST_EXPECT(!mqtt_connection_next(&c, &p) && c.has_error);

    const u8 liar[] = { 0x30, 0x05, 0x00, 0x09, 'a', '/', 'b' };
    c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    c.state = MQTT_CONNECTION_CONNECTED;
    mqtt_test_connection_feed(&c.in, liar, sizeof(liar));
    TEST_EXPECT(!mqtt_connection_next(&c, &p) && c.has_error);
}

void mqtt_test_connection_unsubscribe(void)
{
    u8 in[16];
    u8 out[16];
    MqttConnection c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    c.state = MQTT_CONNECTION_CONNECTED;

    const u8 unsubscribe[] = { 0xA2, 0x07, 0x00, 0x02, 0x00, 0x03, 'a', '/', '#' };
    TEST_EXPECT(mqtt_connection_subscribe(&c, STR("a/#")) == 1);
    mqtt_test_connection_take(&c);
    TEST_EXPECT(mqtt_connection_unsubscribe(&c, STR("a/#")) == 2);
    const str sent = mqtt_test_connection_take(&c);
    TEST_EXPECT_BYTES(sent.data, sent.length, unsubscribe, sizeof(unsubscribe));

    const u8 unsuback[] = { 0xB0, 0x02, 0x00, 0x02 };
    mqtt_test_connection_feed(&c.in, unsuback, sizeof(unsuback));
    MqttPacket p = {0};
    TEST_EXPECT(mqtt_connection_next(&c, &p) && p.type == MQTT_UNSUBACK && p.packet_id == 2);
}

void mqtt_test_connection_connect(void)
{
    const u8 bytes[] = {
        0x20, 0x02, 0x00, 0x00,       // CONNACK: accepted
        0x90, 0x03, 0x00, 0x01, 0x00, // SUBACK: packet id 1, QoS 0
    };
    u8 in[16];
    u8 out[32];
    MqttConnection c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));

    const u8 connect[] = {
        0x10, 0x15, 0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x02, 0x00, 0x0A,
        0x00, 0x09, 't', 'n', 'n', '-', 's', 'i', 'm', '-', '1'
    };
    TEST_EXPECT(mqtt_connection_connect(&c, STR("tnn-sim-1"), 10, STR_EMPTY, STR_EMPTY));
    const str sent = mqtt_test_connection_take(&c);
    TEST_EXPECT_BYTES(sent.data, sent.length, connect, sizeof(connect));

    MqttPacket p = {0};
    mqtt_test_connection_feed(&c.in, bytes, 3);
    TEST_EXPECT(!mqtt_connection_next(&c, &p) && c.state == MQTT_CONNECTION_CONNECTING && !c.has_error);
    mqtt_test_connection_feed(&c.in, bytes + 3, sizeof(bytes) - 3);
    TEST_EXPECT(mqtt_connection_next(&c, &p) && p.type == MQTT_CONNACK && c.state == MQTT_CONNECTION_CONNECTED);
    TEST_EXPECT(mqtt_connection_next(&c, &p) && p.type == MQTT_SUBACK);

    const u8 refused[] = { 0x20, 0x02, 0x00, 0x05 }; // CONNACK: not authorized
    c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    mqtt_test_connection_feed(&c.in, refused, sizeof(refused));
    TEST_EXPECT(!mqtt_connection_next(&c, &p) && c.has_error);

    const u8 suback[] = { 0x90, 0x03, 0x00, 0x01, 0x00 };
    c = mqtt_connection_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    mqtt_test_connection_feed(&c.in, suback, sizeof(suback));
    TEST_EXPECT(!mqtt_connection_next(&c, &p) && c.has_error);
}