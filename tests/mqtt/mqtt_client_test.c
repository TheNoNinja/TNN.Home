#include "test.h"
#include "mqtt/mqtt.h"
#include "platform/platform.h"

internal u64 mqtt_test_client_sent(const PlatformHandle end, u8 *bytes, const u64 capacity)
{
    const i64 count = platform_receive(end, (str){bytes, capacity});
    return count > 0 ? (u64)count : 0;
}

void mqtt_test_client(void)
{
    PlatformHandle ends[2];
    TEST_EXPECT(platform_socket_pair(ends));
    u8 in[16];
    u8 out[16];
    MqttClient c = mqtt_client_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    c.connection.state = MQTT_CONNECTION_CONNECTED;
    c.sock = ends[0];
    loop_watch(&c.loop, ends[0]);
    loop_timer_repeat(&c.loop, c.ping_timer, 20);
    u8 got[16];

    const u8 pingreq[] = { 0xC0, 0x00 };
    TEST_EXPECT(mqtt_client_wait(&c) && loop_has_timer_fired(&c.loop, c.ping_timer));
    u64 length = mqtt_test_client_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, pingreq, sizeof(pingreq));

    const u8 pingresp[] = { 0xD0, 0x00 };
    TEST_EXPECT(platform_send(ends[1], (str){ (u8 *)pingresp, sizeof(pingresp) }) == sizeof(pingresp));
    TEST_EXPECT(mqtt_client_wait(&c) && !loop_has_timer_fired(&c.loop, c.ping_timer));
    MqttPacket p = {0};
    TEST_EXPECT(mqtt_connection_next(&c.connection, &p) && p.type == MQTT_PINGRESP && !c.connection.is_waiting_on_ping);

    const u8 subscribe[] = { 0x82, 0x08, 0x00, 0x01, 0x00, 0x03, 'a', '/', '#', 0x00 };
    TEST_EXPECT(mqtt_client_subscribe(&c, STR("a/#")) && c.connection.packet_id == 1);
    length = mqtt_test_client_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, subscribe, sizeof(subscribe));

    const u8 disconnect[] = { 0xE0, 0x00 };
    TEST_EXPECT(mqtt_client_close(&c, 1) == 0 && c.sock.value == 0);
    length = mqtt_test_client_sent(ends[1], got, sizeof(got));
    TEST_EXPECT_BYTES(got, length, disconnect, sizeof(disconnect));
    platform_close(&ends[1]);

    TEST_EXPECT(platform_socket_pair(ends));
    c = mqtt_client_create(in, sizeof(in), sizeof(in), out, sizeof(out));
    c.sock = ends[0];
    loop_watch(&c.loop, ends[0]);
    loop_timer_repeat(&c.loop, c.ping_timer, 20);
    TEST_EXPECT(mqtt_client_wait(&c) && !c.has_failed);
    TEST_EXPECT(!mqtt_client_wait(&c) && c.has_failed);
    TEST_EXPECT(mqtt_client_close(&c, 1) == 1 && c.sock.value == 0);
    mqtt_test_client_sent(ends[1], got, sizeof(got));
    TEST_EXPECT(mqtt_test_client_sent(ends[1], got, sizeof(got)) == 0);
    platform_close(&ends[1]);
}