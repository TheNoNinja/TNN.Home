#ifndef MQTT_CONNECTION_H
#define MQTT_CONNECTION_H

#include "base/base.h"
#include "mqtt/mqtt_reader.h"
#include "mqtt/mqtt_stream.h"

typedef struct MqttConnection MqttConnection;
struct MqttConnection 
{
    int sock;
    MqttStream in;
    u8 *out;
    u64 out_capacity;
    u16 packet_id;
    b32 is_waiting_on_ping;
    b32 has_error;
};

typedef struct MqttPacket MqttPacket;
struct MqttPacket
{
    u8 type;
    u16 packet_id;
    MqttPublish publish;
};

MqttConnection mqtt_connection_create(u8 *in_buffer, u64 in_capacity, u64 packet_max, u8 *out_buffer, u64 out_capacity);
b32 mqtt_connection_connect(MqttConnection *c, const char *ip, u16 port, str client_id, u16 keep_alive_seconds, str will_topic, str will_payload);
b32 mqtt_connection_receive(MqttConnection *c);
b32 mqtt_connection_publish(const MqttConnection *c, str topic, str payload, b32 retain);
u16 mqtt_connection_subscribe(MqttConnection *c, str filter);
u16 mqtt_connection_unsubscribe(MqttConnection *c, str filter);
b32 mqtt_connection_ping(MqttConnection *c);
b32 mqtt_connection_disconnect(MqttConnection *c);
void mqtt_connection_close(MqttConnection *c);
b32 mqtt_connection_next(MqttConnection *c, MqttPacket *packet);

#endif