#ifndef MQTT_STREAM_H
#define MQTT_STREAM_H

#include "base/base.h"
#include "mqtt/mqtt_reader.h"

typedef struct MqttStream MqttStream;
struct MqttStream 
{
    u8 *data;
    u64 capacity;
    u64 length;
    u64 position;
    u64 packet_max;
    u64 skip;
    u32 skipped;
    b32 has_error;
};

MqttStream mqtt_stream_create(u8 *buffer, u64 capacity, u64 packet_max);
void mqtt_stream_compact(MqttStream *s);
b32 mqtt_stream_next(MqttStream *s, MqttFixedHeader *header, str *body);

#endif