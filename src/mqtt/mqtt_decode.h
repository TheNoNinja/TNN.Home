#ifndef MQTT_DECODE_H
#define MQTT_DECODE_H

#include "base/base.h"

typedef struct MqttFixedHeader MqttFixedHeader;
struct MqttFixedHeader
{
    u8 type;
    u8 flags;
    u32 remaining_length;
};

typedef struct MqttPublish MqttPublish;
struct MqttPublish
{
    str topic;
    str payload;
    b32 is_retained;
    u8 qos;
};

str mqtt_read_str(Reader *r);
u32 mqtt_read_varint(Reader *r);

MqttFixedHeader mqtt_read_fixed_header(Reader *r);
MqttPublish mqtt_read_publish(Reader *r, u8 flags);

#endif