#include "test.h"
#include "mqtt/mqtt.h"

#include <string.h>


internal void mqtt_test_framer_feed(MqttFramer *f, const u8 *bytes, const u64 count)
{
    const str space = mqtt_framer_space(f);
    ASSERT(count <= space.length);
    memcpy(space.data, bytes, count);
    mqtt_framer_received(f, count);
}

void mqtt_test_framer_split(void) 
{
    const u8 packet[] = { 0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n' };
    u8 buffer[16];
    MqttFramer f = mqtt_framer_create(buffer, sizeof(buffer), sizeof(buffer));
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;

    for (u64 i = 0; i + 1 < sizeof(packet); i++)
    {
        mqtt_test_framer_feed(&f, packet + i, 1);
        TEST_EXPECT(!mqtt_framer_next(&f, &header, &body));
    }

    mqtt_test_framer_feed(&f, packet + sizeof(packet) - 1, 1);
    TEST_EXPECT(mqtt_framer_next(&f, &header, &body));
    TEST_EXPECT(header.type == MQTT_PUBLISH && header.flags == MQTT_PUBLISH_FLAG_RETAIN);
    TEST_EXPECT_BYTES(body.data, body.length, packet + 2, 7);
    TEST_EXPECT(!mqtt_framer_next(&f, &header, &body) && !f.has_error);
}

void mqtt_test_framer_burst(void) 
{
    const u8 first[] = {
         0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n',//PUBLISH
         0xD0, 0x00,                                     //PINGRESP
         0x90, 0x03, 0x00                                //Partial SUBACK 
    };
    const u8 last[] = { 0x01, 0x00 };                    //Leftover SUBACK
    const u8 expected[] = { 0x00, 0x01, 0x00 };

    u8 buffer[16];
    MqttFramer f = mqtt_framer_create(buffer, sizeof(buffer), sizeof(buffer));
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;

    mqtt_test_framer_feed(&f, first, sizeof(first));
    TEST_EXPECT(mqtt_framer_next(&f, &header, &body) && header.type == MQTT_PUBLISH && body.length == 7);
    TEST_EXPECT(mqtt_framer_next(&f, &header, &body) && header.type == MQTT_PINGRESP && body.length == 0);
    TEST_EXPECT(!mqtt_framer_next(&f, &header, &body));

    mqtt_test_framer_feed(&f, last, sizeof(last));
    TEST_EXPECT(f.position == 0 && f.length == 5);
    TEST_EXPECT(mqtt_framer_next(&f, &header, &body) && header.type == MQTT_SUBACK);
    TEST_EXPECT_BYTES(body.data, body.length, expected, sizeof(expected));
    TEST_EXPECT(!mqtt_framer_next(&f, &header, &body) && !f.has_error);
}

void mqtt_test_framer_oversized(void)
{
    const u8 big[] = { 0x31, 0x07, 0x00, 0x03, 'a', '/', 'b', 'o', 'n' };
    const u8 after[] = { 0xD0, 0x00 };

    u8 buffer[8];
    MqttFramer f = mqtt_framer_create(buffer, sizeof(buffer), sizeof(buffer));
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;

    mqtt_test_framer_feed(&f, big, 5);
    TEST_EXPECT(!mqtt_framer_next(&f, &header, &body));
    TEST_EXPECT(f.skipped_count == 1 && f.skip_remaining == 4 && f.position == 5);
    mqtt_test_framer_feed(&f, big + 5, 4);
    mqtt_test_framer_feed(&f, after, sizeof(after));
    TEST_EXPECT(mqtt_framer_next(&f, &header, &body) && header.type == MQTT_PINGRESP);
    TEST_EXPECT(!mqtt_framer_next(&f, &header, &body) && !f.has_error && f.skipped_count == 1 && f.skip_remaining == 0);
}

void mqtt_test_framer_malformed(void)
{
    const u8 bad[] = { 0x30, 0x80, 0x80, 0x80, 0x80, 0x01 };
    u8 buffer[16];
    MqttFramer f = mqtt_framer_create(buffer, sizeof(buffer), sizeof(buffer));
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;

    mqtt_test_framer_feed(&f, bad, 4);
    TEST_EXPECT(!mqtt_framer_next(&f, &header, &body) && !f.has_error);
    mqtt_test_framer_feed(&f, bad + 4, 2);
    TEST_EXPECT(!mqtt_framer_next(&f, &header, &body));
    TEST_EXPECT(f.has_error && !mqtt_framer_next(&f, &header, &body));
}