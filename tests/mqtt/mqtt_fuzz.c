#include "base/base.h"
#include "mqtt/mqtt.h"

#include <string.h>

int LLVMFuzzerTestOneInput(const u8 *data, size_t size);

#define MQTT_FUZZ_BUFFER_MAX 64
#define MQTT_FUZZ_PACKET_MAX 32

internal volatile u32 mqtt_fuzz_sink;

internal u32 mqtt_fuzz_touch(const str s)
{
    u32 sum = 0;
    for (u64 i = 0; i < s.length; i++)
    {
        sum += s.data[i];
    }
    return sum;
}

int LLVMFuzzerTestOneInput(const u8 *data, const size_t size)
{
    u8 buffer[MQTT_FUZZ_BUFFER_MAX];
    MqttFramer f = mqtt_framer_create(buffer, sizeof(buffer), MQTT_FUZZ_PACKET_MAX);
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;
    u32 sum = 0;

    const u64 piece = size % 8 + 1;
    u64 position = 0;
    while (position < size && !f.has_error)
    {
        const str space = mqtt_framer_space(&f);
        u64 count = size - position;
        count = count < piece ? count : piece;
        count = count < space.length ? count : space.length;
        memcpy(space.data, data + position, count);
        mqtt_framer_received(&f, count);
        position += count;

        while(mqtt_framer_next(&f, &header, &body))
        {
            ASSERT(body.length == header.remaining_length);
            ASSERT(body.length < MQTT_FUZZ_PACKET_MAX);
            sum += mqtt_fuzz_touch(body);
            MqttPacket p = {0};
            const b32 is_whole = mqtt_packet_parse(header, body, &p);
            ASSERT(p.type == header.type);
            ASSERT(p.publish.topic.length + p.publish.payload.length <= body.length);
            sum += is_whole + mqtt_fuzz_touch(p.publish.topic) + mqtt_fuzz_touch(p.publish.payload);
        }
    }
    mqtt_fuzz_sink = sum;
    return 0;
}