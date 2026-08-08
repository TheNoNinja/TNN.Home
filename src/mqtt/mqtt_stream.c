#include "mqtt/mqtt_stream.h"
#include "mqtt/mqtt.h"
#include "base/base_assert.h"

#include <stdio.h>
#include <string.h>

MqttStream mqtt_stream_create(u8 *buffer, const u64 capacity, const u64 packet_max)
{
    ASSERT(buffer != 0 && packet_max >= MQTT_FIXED_HEADER_MAX && packet_max <= capacity);
    MqttStream s = {0};
    s.data = buffer;
    s.capacity = capacity;
    s.packet_max = packet_max;
    return s;
}

void mqtt_stream_compact(MqttStream *s)
{
    ASSERT(s->position <= s->length);
    const u64 unread = s->length - s->position;
    memmove(s->data, s->data + s->position, unread);
    s->position = 0;
    s->length = unread;
}

b32 mqtt_stream_next(MqttStream *s, MqttFixedHeader *header, str *body)
{
    for (;;)
    {
        if (s->error)
        {
            return 0;
        }

        const u64 unread = s->length - s->position;

        if (s->skip > 0)
        {
            const u64 take = s->skip < unread ? s->skip : unread;
            s->position += take;
            s->skip -= take;
            if (s->skip > 0)
            {
                return 0;
            }
            continue;
        }

        MqttReader r = mqtt_reader_create(s->data + s->position, unread);
        const MqttFixedHeader h = mqtt_read_fixed_header(&r);
        if (r.error)
        {
            if (unread >= MQTT_FIXED_HEADER_MAX)
            {
                fprintf(stderr, "malformed packet: the remaining length has more than 4 bytes\n");
                s->error = 1;
            }
            return 0;
        }

        const u64 packet_length = r.position + h.remaining_length;
        if (packet_length > s->packet_max)
        {
            fprintf(stderr, "skipping a packet of %llu bytes (type %d): the limit is %llu\n", (unsigned long long) packet_length, h.type, (unsigned long long)s->packet_max);
            s->skipped += 1;
            s->skip = packet_length;
            continue;
        }
        
        if (packet_length > unread)
        {
            return 0;
        }

        *header = h;
        *body = mqtt_read_bytes(&r, h.remaining_length);
        s->position += packet_length;
        return 1;
    }
}