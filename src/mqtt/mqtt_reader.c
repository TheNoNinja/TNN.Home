#include "mqtt/mqtt_reader.h"
#include "mqtt/mqtt.h"

MqttReader mqtt_reader_create(const u8 *data, const u64 length)
{
    ASSERT(data != 0);

    MqttReader r = {0};
    r.data = data;
    r.length = length;
    return r;
}

u8 mqtt_read_u8(MqttReader *r) 
{
    if (r->has_error || r->position + 1 > r->length)
    {
        r->has_error = 1;
        return 0;
    }
    const u8 value = r->data[r->position];
    r->position += 1;
    return value;
}

u16 mqtt_read_u16_be(MqttReader *r) 
{
    const u16 high = mqtt_read_u8(r);
    const u16 low = mqtt_read_u8(r);
    return (u16)(high << 8 | low);
}

u64 mqtt_read_u64_be(MqttReader *r)
{
    u64 value = 0;
    for (u32 i = 0; i < 8; i++)
    {
        value = (value << 8) | mqtt_read_u8(r);
    }
    return value;
}

str mqtt_read_bytes(MqttReader *r, const u64 count)
{
    if (r->has_error || r->position + count > r->length)
    {
        r->has_error = 1;
        return STR_EMPTY;
    }

    const str value = { (u8 *)(r->data + r->position), count };
    r->position += count;
    return value;
}

str mqtt_read_str(MqttReader *r)
{
    const u16 length = mqtt_read_u16_be(r);
    return mqtt_read_bytes(r, length);
}

u32 mqtt_read_varint(MqttReader *r) 
{
    u32 value = 0;
    u32 multiplier = 1;
    for (;;)
    {
        if (multiplier > 128 * 128 * 128)
        {
            r->has_error = 1;
            return 0;
        }
        const u8 byte = mqtt_read_u8(r);
        if (r->has_error)
        {
            return 0;
        }
        value += (byte & 127) * multiplier;
        if ((byte & 128) == 0)
        {
            return value;
        }
        multiplier *= 128;
    }
}

MqttFixedHeader mqtt_read_fixed_header(MqttReader *r) 
{
    const u8 first_byte = mqtt_read_u8(r);
    MqttFixedHeader header = {0};
    header.type = first_byte >> 4;
    header.flags = first_byte & 0x0F;
    header.remaining_length = mqtt_read_varint(r);
    return header;
}

MqttPublish mqtt_read_publish(MqttReader *r, const u8 flags)
{
    MqttPublish m = {0};
    m.is_retained = (flags & MQTT_PUBLISH_FLAG_RETAIN) != 0;
    m.qos = (flags >> 1) & 0x03;

    m.topic = mqtt_read_str(r);
    if (m.qos > 0)
    {
        mqtt_read_u16_be(r);
    }

    m.payload = mqtt_read_bytes(r, r->length - r->position);
    return m;
}