#include "base/base_types.h"
#include "base/base_log.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MQTT_DUMP_BROKER_IP "127.0.0.1"
#define MQTT_DUMP_BROKER_PORT 1883
#define MQTT_DUMP_CLIENT_ID "tnn-mqtt-dump"
#define MQTT_DUMP_TOPIC_FILTER "homie/5/#"
#define MQTT_DUMP_KEEP_ALIVE 10
#define MQTT_DUMP_TX_MAX 64
#define MQTT_DUMP_RX_MAX KB(64)
#define MQTT_DUMP_RX_PACKET_MAX KB(64)

internal u8 mqtt_dump_rx_buffer[MQTT_DUMP_RX_MAX];

internal b32 mqtt_dump_handle(const MqttFixedHeader header, const str body)
{
    MqttReader r = mqtt_reader_create(body.data, body.length);

    switch(header.type)
    {
        case MQTT_SUBSCRIBE_ACK:
        {
            const u16 packet_id = mqtt_read_u16_be(&r);
            const u8 return_code = mqtt_read_u8(&r);
            if (r.error || packet_id != 1 || return_code != MQTT_SUBSCRIBE_ACK_QOS_0)
            {
                LOG_ERROR("SUBACK: refused (packet id %d, return code 0x%02x)", packet_id, return_code);
                return 0;
            }
            LOG_INFO("subscribed to %s", MQTT_DUMP_TOPIC_FILTER);
            return 1;
        }
        case MQTT_PUBLISH:
        {
            const MqttPublish m = mqtt_read_publish(&r, header.flags);
            if (r.error)
            {
                LOG_ERROR("malformed PUBLISH of %llu bytes", (unsigned long long)body.length);
                return 0;
            }
            printf("%.*s = %.*s%s\n", STR_FMT(m.topic), STR_FMT(m.payload), m.retain ? " (retained)" : "");
            return 1;
        }
        case MQTT_PINGRESP:
            return 1;
        default:
            LOG_WARN("unexpected packet type %d, ignored", header.type);
            return 1;
    }
}

internal b32 mqtt_dump_receive(MqttClient *c)
{
    if (!mqtt_client_receive(c))
    {
        return 0;
    }

    MqttFixedHeader header = {0};
    str body = STR_EMPTY;
    while (mqtt_stream_next(&c->in, &header, &body))
    {
        if (!mqtt_dump_handle(header, body))
        {
            return 0;
        }
    }

    const u64 waiting = c->in.length - c->in.position;
    if (waiting > 0)
    {
        LOG_INFO("(%llu bytes of a cut-off packet wait for the rest)", (unsigned long long) waiting);
    }
    return !c->in.error;
}

int main(const int argc, const char **argv) 
{
    setvbuf(stdout, 0, _IOLBF, 0);

    u16 port = MQTT_DUMP_BROKER_PORT;
    if(argc > 1)
    {
        char *end = 0;
        const long value = strtol(argv[1], &end, 10);
        if (*end != '\0' || value < 1 || value > 65535)
        {
            LOG_ERROR("usage: %s [port(default '%d')]", argv[0], MQTT_DUMP_BROKER_PORT);
            return 1;
        }
        port = (u16)value;
    }

    MqttClient client = mqtt_client_create(mqtt_dump_rx_buffer, sizeof(mqtt_dump_rx_buffer), MQTT_DUMP_RX_PACKET_MAX);
    if (!mqtt_client_connect(&client, MQTT_DUMP_BROKER_IP, port, STR_LITERAL(MQTT_DUMP_CLIENT_ID), MQTT_DUMP_KEEP_ALIVE, STR_EMPTY, STR_EMPTY))
    {
        return 1;
    }

    u8 packet[MQTT_DUMP_TX_MAX];
    MqttWriter w = mqtt_writer_create(packet, sizeof(packet));
    mqtt_write_subscribe(&w, 1, STR_LITERAL(MQTT_DUMP_TOPIC_FILTER));
    b32 ok = mqtt_client_send(&client, &w);

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
            w = mqtt_writer_create(packet, sizeof(packet));
            mqtt_write_pingreq(&w);
            ok = ok && mqtt_client_send(&client, &w);
            loop_timer_start(&loop, ping, MQTT_DUMP_KEEP_ALIVE * 1000);
        }
    }

    ok = ok && loop.stop_signal != 0;
    if (ok)
    {
        LOG_INFO("signal %d (%s): disconnecting", loop.stop_signal, strsignal(loop.stop_signal));
        w = mqtt_writer_create(packet, sizeof(packet));
        mqtt_write_disconnect(&w);
        ok = mqtt_client_send(&client, &w);
    }

    close(client.sock);
    return ok ? 0 : 1;
}