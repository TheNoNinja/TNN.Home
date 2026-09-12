#include "base/base_assert.h"
#include "base/base_types.h"
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
    MqttStream s = mqtt_stream_create(buffer, sizeof(buffer), MQTT_FUZZ_PACKET_MAX);
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;
    u32 sum = 0;

    const u64 piece = size % 8 + 1;
    u64 position = 0;
    while (position < size && !s.has_error)
    {
        mqtt_stream_compact(&s);
        u64 count = size - position;
        count = count < piece ? count : piece;
        count = count < s.capacity - s.length ? count : s.capacity - s.length;
        memcpy(s.data + s.length, data + position, count);
        s.length += count;
        position += count;

        while(mqtt_stream_next(&s, &header, &body))
        {
            ASSERT(body.length == header.remaining_length);
            ASSERT(body.length < MQTT_FUZZ_PACKET_MAX);
            sum += mqtt_fuzz_touch(body);
            if (header.type == MQTT_PUB)
            {
                MqttReader r = mqtt_reader_create(body.data, body.length);
                const MqttPublish m = mqtt_read_publish(&r, header.flags);
                ASSERT(r.has_error || m.topic.length + m.payload.length <= body.length);
                sum += mqtt_fuzz_touch(m.topic) + mqtt_fuzz_touch(m.payload);
            }
        }
    }
    mqtt_fuzz_sink = sum;
    return 0;
}