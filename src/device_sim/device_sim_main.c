#include "base/base_types.h"
#include "base/base_arena.h"
#include "base/base_log.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEVICE_SIM_BROKER_IP     "127.0.0.1"
#define DEVICE_SIM_BROKER_PORT   1883
#define DEVICE_SIM_ID            "tnn-sim-1"
#define DEVICE_SIM_TOPIC         "homie/5/" DEVICE_SIM_ID "/"
#define DEVICE_SIM_KEEP_ALIVE    10
#define DEVICE_SIM_PACKET_MAX    256
#define DEVICE_SIM_RX_MAX        KB(8)
#define DEVICE_SIM_RX_PACKET_MAX KB(4)


internal u8 device_sim_memory[KB(16)];

internal b32 device_sim_publish(Arena *arena, const MqttClient *c, const str topic, const str payload, const b32 retain)
{
    const u64 mark = arena->position;
    u8 *buffer = ARENA_PUSH_ARRAY(arena, u8, DEVICE_SIM_PACKET_MAX);
    if(!buffer)
    {
        return 0;
    }

    MqttWriter w = mqtt_writer_create(buffer, DEVICE_SIM_PACKET_MAX);
    mqtt_write_publish(&w, topic, payload, retain);
    printf("publish %.*s = %.*s\n", STR_FMT(topic), STR_FMT(payload));
    const b32 ok = mqtt_client_send(c, &w);
    arena_pop_to(arena, mark);
    return ok;
}

internal b32 device_sim_run(Loop *l, MqttClient *c, u8 *buffer)
{
    const u64 interval_ms = DEVICE_SIM_KEEP_ALIVE * 1000;
    const u32 ping = loop_timer_add(l);
    loop_timer_start(l, ping, interval_ms);
    b32 ping_answered = 1;

    loop_watch(l, STDIN_FILENO);
    loop_watch(l, c->sock);

    while(loop_wait(l))
    {
        if (loop_readable(l, STDIN_FILENO))
        {
            return 1;
        }

        if (loop_readable(l, c->sock))
        {
            if (!mqtt_client_receive(c))
            {
                return 0;
            }
            MqttFixedHeader header = {0};
            str body = STR_EMPTY;
            while (mqtt_stream_next(&c->in, &header, &body))
            {
                if(header.type != MQTT_PINGRESP)
                {
                    LOG_ERROR("unexpected packet from the broker: type %d", header.type);
                    return 0;
                }
                printf("PINGRESP\n");
                ping_answered = 1;  
            }
            if(c->in.error)
            {
                return 0;
            }          
        }

        if (loop_timer_fired(l, ping))
        {
            if (!ping_answered)
            {
                LOG_ERROR("no PINGRESP within %ds", DEVICE_SIM_KEEP_ALIVE);
                return 0;
            }

            MqttWriter w = mqtt_writer_create(buffer, DEVICE_SIM_PACKET_MAX);
            mqtt_write_pingreq(&w);
            printf("PINGREQ\n");
            if (!mqtt_client_send(c, &w))
            {
                return 0;
            }
            ping_answered = 0;
            loop_timer_start(l, ping, interval_ms);
        }
    }

    if (l->stop_signal == 0)
    {
        return 0;
    }
    LOG_INFO("signal %d (%s): disconnecting", l->stop_signal, strsignal(l->stop_signal));
    return 1;
}

int main(const int argc, const char **argv)
{
    Arena arena = arena_create(device_sim_memory, sizeof(device_sim_memory));

    u8 *rx_buffer = ARENA_PUSH_ARRAY(&arena, u8, DEVICE_SIM_RX_MAX);
    if (!rx_buffer)
    {
        return 1;
    }

    u16 port = DEVICE_SIM_BROKER_PORT;
    if(argc > 1)
    {
        char *end = 0;
        const long value = strtol(argv[1], &end, 10);
        if (*end != '\0' || value < 1 || value > 65535)
        {
            LOG_ERROR("usage: %s [port(default '%d')]", argv[0], DEVICE_SIM_BROKER_PORT);
            return 1;
        }
        port = (u16)value;
    }

    MqttClient client = mqtt_client_create(rx_buffer, DEVICE_SIM_RX_MAX, DEVICE_SIM_RX_PACKET_MAX);
    if (!mqtt_client_connect(&client, DEVICE_SIM_BROKER_IP, port, STR_LITERAL(DEVICE_SIM_ID), DEVICE_SIM_KEEP_ALIVE, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("lost")))
    {
        return 1;
    }

    u8* buffer = ARENA_PUSH_ARRAY(&arena, u8, DEVICE_SIM_PACKET_MAX);
    if (!buffer)
    {
        goto error_handling;
    }

    b32 ok = 1;
    ok = ok && device_sim_publish(&arena, &client, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("init"), 1);
    ok = ok && device_sim_publish(&arena, &client, STR_LITERAL(DEVICE_SIM_TOPIC "$description"), STR_LITERAL("{\"homie\":\"5.0\",\"version\":1,\"name\":\"TNN simulator 1\"}"), 1);
    ok = ok && device_sim_publish(&arena, &client, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("ready"), 1);

    if (!ok)
    {
        goto error_handling;
    }

    printf("press Enter or Ctrl+C to disconnect\n");
    Loop loop = loop_create();
    loop_catch_signals();
    if (!device_sim_run(&loop, &client, buffer))
    {
        goto error_handling;
    }

    ok = device_sim_publish(&arena, &client, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("disconnected"), 1);
    MqttWriter w = mqtt_writer_create(buffer, DEVICE_SIM_PACKET_MAX);
    mqtt_write_disconnect(&w);
    ok = ok && mqtt_client_send(&client, &w);

    close(client.sock);
    printf("disconnected\n");
    return ok ? 0 : 1;

    error_handling:
    if (client.sock >= 0)
    {
        close(client.sock);
    }
    return 1;
}