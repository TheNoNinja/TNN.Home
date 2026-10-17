#include "base/base.h"
#include "mqtt/mqtt.h"

#include <stdio.h>

#define MQTT_DUMP_BROKER_IP "127.0.0.1"
#define MQTT_DUMP_BROKER_PORT 1883
#define MQTT_DUMP_CLIENT_ID "tnn-mqtt-dump"
#define MQTT_DUMP_TOPIC_FILTER "homie/5/#"
#define MQTT_DUMP_KEEP_ALIVE 10
#define MQTT_DUMP_TX_MAX 64
#define MQTT_DUMP_RX_MAX KB(64)
#define MQTT_DUMP_RX_PACKET_MAX KB(64)

internal u8 mqtt_dump_rx_buffer[MQTT_DUMP_RX_MAX];
internal u8 mqtt_dump_tx_buffer[MQTT_DUMP_TX_MAX];

internal void mqtt_dump_print(MqttClient *c)
{
    MqttPacket p = {0};
    while (mqtt_connection_next(&c->connection, &p))
    {
        if (p.type == MQTT_PUBLISH)
        {
            const MqttPublish m = p.publish;
            printf("%.*s = %.*s%s\n", STR_FMT(m.topic), STR_FMT(m.payload), m.is_retained ? " (retained)" : "");
        }
    }

    const u64 waiting = c->connection.in.length - c->connection.in.position;
    if (waiting > 0)
    {
        LOG_INFO("(%llu bytes of a cut-off packet wait for the rest)", (unsigned long long) waiting);
    }
}

int main(const int argc, const char **argv) 
{
    setvbuf(stdout, 0, _IOLBF, 0);

    u64 port = MQTT_DUMP_BROKER_PORT;
    if(argc > 1 && !cstr_to_u64_range(argv[1], 1, 65535, &port))
    {
        LOG_ERROR("usage: %s [port(default '%d')]", argv[0], MQTT_DUMP_BROKER_PORT);
        return 1;
    }

    MqttClient client = mqtt_client_create(mqtt_dump_rx_buffer, sizeof(mqtt_dump_rx_buffer), MQTT_DUMP_RX_PACKET_MAX, mqtt_dump_tx_buffer, sizeof(mqtt_dump_tx_buffer));
    if (!mqtt_client_connect(&client, MQTT_DUMP_BROKER_IP, (u16)port, STR(MQTT_DUMP_CLIENT_ID), MQTT_DUMP_KEEP_ALIVE, STR_EMPTY, STR_EMPTY))
    {
        return 1;
    }
    
    const b32 ok = mqtt_client_subscribe(&client, STR(MQTT_DUMP_TOPIC_FILTER));
    while (ok && mqtt_client_wait(&client))
    {
        mqtt_dump_print(&client);
    }

    return mqtt_client_close(&client, ok);
}