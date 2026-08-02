#ifndef MQTT_READER_H
#define MQTT_READER_H

#include "base/base_types.h"

typedef struct MqttReader MqttReader;
struct MqttReader 
{
    const u8 *data;
    u64 length;
    u64 position;
    b32 error;
};

MqttReader mqtt_reader_create(const u8 *data, u64 length);

u8 mqtt_read_u8(MqttReader *r);
u16 mqtt_read_u16_be(MqttReader *r);
str mqtt_read_bytes(MqttReader *r, u64 count);
str mqtt_read_str(MqttReader *r);
u32 mqtt_read_varint(MqttReader *r);

#endif