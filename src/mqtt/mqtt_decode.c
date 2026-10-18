#include "mqtt/mqtt_decode.h"
#include "mqtt/mqtt_protocol.h"

str mqtt_read_str(Reader *r)
{
    const u16 length = read_u16_be(r);
    return read_bytes(r, length);
}

u32 mqtt_read_varint(Reader *r) 
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
        const u8 byte = read_u8(r);
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

MqttFixedHeader mqtt_read_fixed_header(Reader *r) 
{
    const u8 first_byte = read_u8(r);
    MqttFixedHeader header = {0};
    header.type = first_byte >> 4;
    header.flags = first_byte & 0x0F;
    header.remaining_length = mqtt_read_varint(r);
    return header;
}

MqttPublish mqtt_read_publish(Reader *r, const u8 flags)
{
    MqttPublish m = {0};
    m.is_retained = (flags & MQTT_PUBLISH_FLAG_RETAIN) != 0;
    m.qos = (flags >> 1) & 0x03;

    m.topic = mqtt_read_str(r);
    if (m.qos > 0)
    {
        read_u16_be(r);
    }

    m.payload = read_bytes(r, r->length - r->position);
    return m;
}

b32 mqtt_packet_parse(const MqttFixedHeader header, const str body, MqttPacket *packet)
{
    Reader r = reader_create(body.data, body.length);
    *packet = (MqttPacket){0};
    packet->type = header.type;

    switch(header.type)
    {
        case MQTT_CONNACK:
        {
            read_u8(&r);
            packet->return_code = read_u8(&r);
            return !r.has_error && body.length == 2;
        }
        case MQTT_PUBLISH:
        {
            packet->publish = mqtt_read_publish(&r, header.flags);
            break;
        }
        case MQTT_SUBACK:
        {
            packet->packet_id = read_u16_be(&r);
            packet->return_code = read_u8(&r);
            break;
        }
        case MQTT_UNSUBACK:
        {
            packet->packet_id = read_u16_be(&r);
            break;
        }
        default:
        {
            break;
        }
    }
    return !r.has_error;
}