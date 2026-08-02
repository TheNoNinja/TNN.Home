#include "mqtt/mqtt_writer.h"
#include "mqtt/mqtt.h"
#include "base/base_assert.h"

#include <string.h>

MqttWriter mqtt_writer_create(u8 *buffer, const u64 capacity)
{
    ASSERT(buffer != 0);

    MqttWriter w = {0};
    w.data = buffer;
    w.capacity = capacity;
    return w;
}


void mqtt_write_u8(MqttWriter *w, const u8 value)
{
    if (w->length + 1 > w->capacity)
    {
        w->overflow = 1;
        return;
    }
    w->data[w->length] = value;
    w->length += 1;
}

void mqtt_write_u16_be(MqttWriter *w, const u16 value)
{
    mqtt_write_u8(w, (u8)(value >> 8));
    mqtt_write_u8(w, (u8)value & 0xFF);
}

void mqtt_write_bytes(MqttWriter *w, const str value)
{
    if (w->length + value.length > w->capacity)
    {
        w->overflow = 1;
        return;
    }
    memcpy(w->data + w->length, value.data, value.length);
    w->length += value.length;
}

void mqtt_write_str(MqttWriter *w, str value)
{
    if (value.length > 0xFFFF)
    {
        w->overflow = 1;
        return;
    }
    mqtt_write_u16_be(w, (u16)value.length);
    mqtt_write_bytes(w, value);
}

void mqtt_write_varint(MqttWriter *w, u32 value)
{
    if (value > MQTT_VARINT_MAX)
    {
        w->overflow = 1;
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
        mqtt_write_u8(w, byte);
    } while (value > 0);
}


void mqtt_write_connect(MqttWriter *w, const str client_id, const u16 keep_alive_seconds, const str will_topic, const str will_payload)
{
    const str protocol_name = STR_LITERAL("MQTT");
    const b32 has_will = will_topic.length > 0;

    u32 remaining = 2 + (u32)protocol_name.length + 1 + 1 + 2 + 2 + (u32)client_id.length;

    u8 flags = MQTT_CONNECT_FLAG_CLEAN_SESSION;
    if (has_will)
    {
        remaining += 2 + (u32)will_topic.length + 2 + (u32)will_payload.length;
        flags = flags | MQTT_CONNECT_FLAG_WILL | MQTT_CONNECT_FLAG_WILL_RETAIN;
    }

    mqtt_write_u8(w, MQTT_CONNECT << 4);
    mqtt_write_varint(w, remaining);
    mqtt_write_str(w, protocol_name);
    mqtt_write_u8(w, MQTT_PROTOCOL_3_1_1);
    mqtt_write_u8(w, flags);
    mqtt_write_u16_be(w, keep_alive_seconds);
    mqtt_write_str(w, client_id);
    if (has_will)
    {
        mqtt_write_str(w, will_topic);
        mqtt_write_str(w, will_payload);
    }
}

void mqtt_write_subscribe(MqttWriter *w, const u16 packet_id, const str topic_filter) 
{
    const u32 remaining = 2 + 2 + (u32)topic_filter.length + 1;

    mqtt_write_u8(w, MQTT_SUBSCRIBE << 4 | MQTT_SUBSCRIBE_FLAG);
    mqtt_write_varint(w, remaining);
    mqtt_write_u16_be(w, packet_id);
    mqtt_write_str(w, topic_filter);
    mqtt_write_u8(w, 0);
}

void mqtt_write_publish(MqttWriter *w, const str topic, const str payload, const b32 retain)
{
    const u32 remaining = 2 + (u32)topic.length + (u32)payload.length;
    u8 first_byte = MQTT_PUBLISH << 4;
    if (retain)
    {
        first_byte = first_byte | MQTT_PUBLISH_FLAG_RETAIN;
    }
    mqtt_write_u8(w, first_byte);
    mqtt_write_varint(w, remaining);
    mqtt_write_str(w, topic);
    mqtt_write_bytes(w, payload);
}

void mqtt_write_pingreq(MqttWriter *w)
{
    mqtt_write_u8(w, MQTT_PINGREQ << 4);
    mqtt_write_u8(w, 0);
}

void mqtt_write_disconnect(MqttWriter *w)
{
    mqtt_write_u8(w, MQTT_DISCONNECT << 4);
    mqtt_write_u8(w, 0);
}