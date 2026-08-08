#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "base/base_types.h"
#include "mqtt/mqtt_stream.h"
#include "mqtt/mqtt_writer.h"

typedef struct MqttClient MqttClient;
struct MqttClient 
{
    int sock;
    MqttStream in;
};

MqttClient mqtt_client_create(u8 *buffer, u64 capacity, u64 packet_max);
b32 mqtt_client_connect(MqttClient *c, const char *ip, u16 port, str client_id, u16 keep_alive_seconds, str will_topic, str will_payload);
i64 mqtt_client_receive(MqttClient *c);
b32 mqtt_client_send(MqttClient *c, const MqttWriter *w);

#endif