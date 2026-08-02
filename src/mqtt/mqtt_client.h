#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "base/base_types.h"
#include "mqtt/mqtt_writer.h"

typedef struct MqttClient MqttClient;
struct MqttClient {
    int sock_ref;
};

MqttClient mqtt_client_connect(const char *ip, u16 port, str client_id, u16 keep_alive_seconds, str will_topic, str will_payload);
b32 mqtt_client_send(MqttClient *c, const MqttWriter *w);

#endif