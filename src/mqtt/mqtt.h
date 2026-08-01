#ifndef MQTT_H
#define MQTT_H

#include "base/base_types.h"

#define MQTT_CONNECT     1
#define MQTT_CONNECT_ACK 2
#define MQTT_PUBLISH     3
#define MQTT_DISCONNECT  14

#define MQTT_VARINT_MAX  268435455

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

void mqtt_write_connect(MqttWriter *w, str client_id, u16 keep_alive_seconds);
void mqtt_write_publish(MqttWriter *w, str topic, str payload, b32 retain);
void mqtt_write_disconnect(MqttWriter *w);

#endif