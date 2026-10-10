#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "base/base.h"
#include "loop/loop.h"
#include "mqtt/mqtt_connection.h"

typedef struct MqttClient MqttClient;
struct MqttClient 
{
    MqttConnection connection;
    Loop loop;
    u32 ping_timer;
    b32 has_failed;
};

MqttClient mqtt_client_create(u8 *in_buffer, u64 in_capacity, u64 packet_max, u8 *out_buffer, u64 out_capacity);
b32 mqtt_client_connect(MqttClient *c, const char *ip, u16 port, str client_id, u16 keep_alive_seconds, str will_topic, str will_payload);
b32 mqtt_client_wait(MqttClient *c);
b32 mqtt_client_subscribe(MqttClient *c, str topic_filter);
b32 mqtt_client_publish(MqttClient *c, str topic, str payload, b32 retain);
int mqtt_client_close(MqttClient *c, b32 ok);

#endif