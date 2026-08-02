#ifndef MQTT_WRITER_H
#define MQTT_WRITER_H

#include "base/base_types.h"

typedef struct MqttWriter MqttWriter;
struct MqttWriter
{
    u8 *data;
    u64 capacity;
    u64 length;
    b32 overflow;
};

MqttWriter mqtt_writer_create(u8 *buffer, u64 capacity);

void mqtt_write_u8(MqttWriter *w, u8 value);
void mqtt_write_u16_be(MqttWriter *w, u16 value);
void mqtt_write_bytes(MqttWriter *w, str value);
void mqtt_write_str(MqttWriter *w, str value);
void mqtt_write_varint(MqttWriter *w, u32 value);

void mqtt_write_connect(MqttWriter *w, str client_id, u16 keep_alive_seconds, str will_topic, str will_payload);
void mqtt_write_subscribe(MqttWriter *w, u16 packet_id, str topic_filter);
void mqtt_write_publish(MqttWriter *w, str topic, str payload, b32 retain);
void mqtt_write_pingreq(MqttWriter *w);
void mqtt_write_disconnect(MqttWriter *w);

#endif