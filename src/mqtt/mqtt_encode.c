#include "mqtt/mqtt_encode.h"
#include "mqtt/mqtt_protocol.h"

void mqtt_write_str(Writer *w, const str value)
{
    if (value.length > 0xFFFF)
    {
        w->has_overflown = 1;
        return;
    }
    write_u16_be(w, (u16)value.length);
    write_bytes(w, value);
}

void mqtt_write_varint(Writer *w, u32 value)
{
    if (value > MQTT_VARINT_MAX)
    {
        w->has_overflown = 1;
        return;
    }

    do
    {
        u8 byte = (u8)(value % 128);
        value = value / 128;
        if (value > 0)
        {
            byte = byte | 128;
        }
        write_u8(w, byte);
    } while (value > 0);
}


void mqtt_write_connect(Writer *w, const str client_id, const u16 keep_alive_seconds, const str will_topic, const str will_payload)
{
    const str protocol_name = STR("MQTT");
    const b32 has_will = will_topic.length > 0;

    u32 remaining = 2 + (u32)protocol_name.length + 1 + 1 + 2 + 2 + (u32)client_id.length;

    u8 flags = MQTT_CONNECT_FLAG_CLEAN_SESSION;
    if (has_will)
    {
        remaining += 2 + (u32)will_topic.length + 2 + (u32)will_payload.length;
        flags = flags | MQTT_CONNECT_FLAG_WILL | MQTT_CONNECT_FLAG_WILL_RETAIN;
    }

    write_u8(w, MQTT_CONNECT << 4);
    mqtt_write_varint(w, remaining);
    mqtt_write_str(w, protocol_name);
    write_u8(w, MQTT_PROTOCOL_3_1_1);
    write_u8(w, flags);
    write_u16_be(w, keep_alive_seconds);
    mqtt_write_str(w, client_id);
    if (has_will)
    {
        mqtt_write_str(w, will_topic);
        mqtt_write_str(w, will_payload);
    }
}

void mqtt_write_publish(Writer *w, const str topic, const str payload, const b32 retain)
{
    const u32 remaining = 2 + (u32)topic.length + (u32)payload.length;
    u8 first_byte = MQTT_PUBLISH << 4;
    if (retain)
    {
        first_byte = first_byte | MQTT_PUBLISH_FLAG_RETAIN;
    }
    write_u8(w, first_byte);
    mqtt_write_varint(w, remaining);
    mqtt_write_str(w, topic);
    write_bytes(w, payload);
}

void mqtt_write_subscribe(Writer *w, const u16 packet_id, const str topic_filter) 
{
    const u32 remaining = 2 + 2 + (u32)topic_filter.length + 1;

    write_u8(w, MQTT_SUBSCRIBE << 4 | MQTT_SUBSCRIBE_FLAG);
    mqtt_write_varint(w, remaining);
    write_u16_be(w, packet_id);
    mqtt_write_str(w, topic_filter);
    write_u8(w, 0);
}

void mqtt_write_unsubscribe(Writer *w, const u16 packet_id, const str topic_filter)
{
    const u32 remaining = 2 + 2 + (u32)topic_filter.length;

    write_u8(w, MQTT_UNSUBSCRIBE << 4 | MQTT_UNSUBSCRIBE_FLAG);
    mqtt_write_varint(w, remaining);
    write_u16_be(w, packet_id);
    mqtt_write_str(w, topic_filter);
}

void mqtt_write_pingreq(Writer *w)
{
    write_u8(w, MQTT_PINGREQ << 4);
    write_u8(w, 0);
}

void mqtt_write_disconnect(Writer *w)
{
    write_u8(w, MQTT_DISCONNECT << 4);
    write_u8(w, 0);
}