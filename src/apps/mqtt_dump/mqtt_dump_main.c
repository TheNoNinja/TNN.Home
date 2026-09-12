#include "base/base.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"

#include <stdio.h>
#include <string.h>

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

internal b32 mqtt_dump_receive(MqttClient *c)
{
    if (!mqtt_client_receive(c))
    {
        return 0;
    }

    MqttPacket p = {0};
    while (mqtt_client_next(c, &p))
    {
        if (p.type == MQTT_PUBLISH)
        {
            const MqttPublish m = p.publish;
            printf("%.*s = %.*s%s\n", STR_FMT(m.topic), STR_FMT(m.payload), m.is_retained ? " (retained)" : "");
        }
    }

    const u64 waiting = c->in.length - c->in.position;
    if (waiting > 0)
    {
        LOG_INFO("(%llu bytes of a cut-off packet wait for the rest)", (unsigned long long) waiting);
    }
    return !c->has_error;
}

int main(const int argc, const char **argv) 
{
    setvbuf(stdout, 0, _IOLBF, 0);

    u64 port = MQTT_DUMP_BROKER_PORT;
    if(argc > 1 && !(str_to_u64(str_from_cstr(argv[1]), &port) && port >= 1 && port <= 65535))
    {
        LOG_ERROR("usage: %s [port(default '%d')]", argv[0], MQTT_DUMP_BROKER_PORT);
        return 1;
    }

    MqttClient client = mqtt_client_create(mqtt_dump_rx_buffer, sizeof(mqtt_dump_rx_buffer), MQTT_DUMP_RX_PACKET_MAX, mqtt_dump_tx_buffer, sizeof(mqtt_dump_tx_buffer));
    if (!mqtt_client_connect(&client, MQTT_DUMP_BROKER_IP, (u16)port, STR(MQTT_DUMP_CLIENT_ID), MQTT_DUMP_KEEP_ALIVE, STR_EMPTY, STR_EMPTY))
    {
        return 1;
    }

    b32 ok = mqtt_client_subscribe(&client, STR(MQTT_DUMP_TOPIC_FILTER)) != 0;

    Loop loop = loop_create();
    loop_catch_signals();
    loop_watch(&loop, client.sock);
    const u32 ping = loop_timer_add(&loop);
    loop_timer_start(&loop, ping, MQTT_DUMP_KEEP_ALIVE * 1000);

    while (ok && loop_wait(&loop))
    {
        if (loop_readable(&loop, client.sock))
        {
            ok = mqtt_dump_receive(&client);
        }

        if (loop_timer_fired(&loop, ping))
        {
            ok = ok && mqtt_client_ping(&client);
            loop_timer_start(&loop, ping, MQTT_DUMP_KEEP_ALIVE * 1000);
        }
    }

    ok = ok && loop.stop_signal != 0;
    if (ok)
    {
        LOG_INFO("signal %d (%s): disconnecting", loop.stop_signal, strsignal(loop.stop_signal));
        return mqtt_client_disconnect(&client) ? 0 : 1;
    }

    mqtt_client_close(&client);
    return 1;
}