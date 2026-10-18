#include "mqtt/mqtt_framer.h"
#include "mqtt/mqtt_protocol.h"

#include <string.h>

MqttFramer mqtt_framer_create(u8 *buffer, const u64 capacity, const u64 packet_max)
{
    ASSERT(buffer != 0 && packet_max >= MQTT_FIXED_HEADER_MAX && packet_max <= capacity);
    MqttFramer f = {0};
    f.data = buffer;
    f.capacity = capacity;
    f.packet_max = packet_max;
    return f;
}

void mqtt_framer_compact(MqttFramer *f)
{
    ASSERT(f->position <= f->length);
    const u64 unread = f->length - f->position;
    memmove(f->data, f->data + f->position, unread);
    f->position = 0;
    f->length = unread;
}

b32 mqtt_framer_next(MqttFramer *f, MqttFixedHeader *header, str *body)
{
    for (;;)
    {
        if (f->has_error)
        {
            return 0;
        }

        const u64 unread = f->length - f->position;

        if (f->skip_remaining > 0)
        {
            const u64 take = f->skip_remaining < unread ? f->skip_remaining : unread;
            f->position += take;
            f->skip_remaining -= take;
            if (f->skip_remaining > 0)
            {
                return 0;
            }
            continue;
        }

        Reader r = reader_create(f->data + f->position, unread);
        const MqttFixedHeader h = mqtt_read_fixed_header(&r);
        if (r.has_error)
        {
            if (unread >= MQTT_FIXED_HEADER_MAX)
            {
                LOG_ERROR("malformed packet: the remaining length has more than 4 bytes");
                f->has_error = 1;
            }
            return 0;
        }

        const u64 packet_length = r.position + h.remaining_length;
        if (packet_length > f->packet_max)
        {
            LOG_WARN("skipping a packet of %llu bytes (type %d): the limit is %llu", (unsigned long long) packet_length, h.type, (unsigned long long)f->packet_max);
            f->skipped_count += 1;
            f->skip_remaining = packet_length;
            continue;
        }
        
        if (packet_length > unread)
        {
            return 0;
        }

        *header = h;
        *body = read_bytes(&r, h.remaining_length);
        f->position += packet_length;
        return 1;
    }
}