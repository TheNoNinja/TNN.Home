#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "base/base.h"
#include "mqtt/mqtt_reader.h"
#include "mqtt/mqtt_stream.h"

typedef struct MqttClient MqttClient;
struct MqttClient 
{
    int sock;
    MqttStream in;
    u8 *out;
    u64 out_capacity;
    u16 packet_id;
    b32 ping_waiting;
    b32 error;
};

MqttClient mqtt_client_create(u8 *in_buffer, u64 in_capacity, u64 packet_max, u8 *out_buffer, u64 out_capacity);
b32 mqtt_client_connect(MqttClient *c, const char *ip, u16 port, str client_id, u16 keep_alive_seconds, str will_topic, str will_payload);
b32 mqtt_client_receive(MqttClient *c);
b32 mqtt_client_publish(const MqttClient *c, str topic, str payload, b32 retain);
u16 mqtt_client_subscribe(MqttClient *c, str filter);
b32 mqtt_client_ping(MqttClient *c);
b32 mqtt_client_disconnect(MqttClient *c);
void mqtt_client_close(MqttClient *c);

typedef struct MqttPacket MqttPacket;
struct MqttPacket
{
    u8 type;
    u16 packet_id;
    MqttPublish publish;
};

b32 mqtt_client_next(MqttClient *c, MqttPacket *packet);

#endif