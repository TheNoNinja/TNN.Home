#ifndef MQTT_FRAMER_H
#define MQTT_FRAMER_H

#include "base/base.h"
#include "mqtt/mqtt_decode.h"

typedef struct MqttFramer MqttFramer;
struct MqttFramer 
{
    u8 *data;
    u64 capacity;
    u64 length;
    u64 position;
    u64 packet_max;
    u64 skip_remaining;
    u32 skipped_count;
    b32 has_error;
};

MqttFramer mqtt_framer_create(u8 *buffer, u64 capacity, u64 packet_max);
void mqtt_framer_compact(MqttFramer *f);
b32 mqtt_framer_next(MqttFramer *f, MqttFixedHeader *header, str *body);

#endif