#ifndef MQTT_READER_H
#define MQTT_READER_H

#include "base/base.h"

typedef struct MqttReader MqttReader;
struct MqttReader 
{
    const u8 *data;
    u64 length;
    u64 position;
    b32 has_error;
};

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

MqttReader mqtt_reader_create(const u8 *data, u64 length);

u8 mqtt_read_u8(MqttReader *r);
u16 mqtt_read_u16_be(MqttReader *r);
u64 mqtt_read_u64_be(MqttReader *r);
str mqtt_read_bytes(MqttReader *r, u64 count);
str mqtt_read_str(MqttReader *r);
u32 mqtt_read_varint(MqttReader *r);

MqttFixedHeader mqtt_read_fixed_header(MqttReader *r);
MqttPublish mqtt_read_publish(MqttReader *r, u8 flags);

#endif