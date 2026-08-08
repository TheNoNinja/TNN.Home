#include "base/base_types.h"
#include "base/base_arena.h"
#include "mqtt/mqtt.h"

#include <poll.h>
#include <stdio.h>
#include <time.h>
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

internal b32 device_sim_publish(Arena *arena, MqttClient *c, const str topic, const str payload, const b32 retain)
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

internal u64 device_sim_now_ms(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

internal b32 device_sim_wait_for_enter(MqttClient *c, u8 *buffer)
{
    const u64 interval_ms = DEVICE_SIM_KEEP_ALIVE * 1000;
    u64 next_ping_ms = device_sim_now_ms() + interval_ms;
    b32 ping_answered = 1;

    for(;;)
    {
        struct pollfd fds[2] = {0};
        fds[0].fd = STDIN_FILENO;
        fds[0].events = POLLIN;
        fds[1].fd = c->sock;
        fds[1].events = POLLIN;

        const u64 now_ms = device_sim_now_ms();
        const int timeout_ms = now_ms < next_ping_ms ? (int)(next_ping_ms - now_ms) : 0;
        if (poll(fds, ARRAY_COUNT(fds), timeout_ms) < 0)
        {
            perror("poll");
            return 0;
        }

        if (fds[0].revents)
        {
            return 1;
        }

        if (fds[1].revents)
        {
            if (mqtt_client_receive(c) <= 0)
            {
                fprintf(stderr, "the broker closed the connection\n");
                return 0;
            }
            MqttFixedHeader header = {0};
            str body = STR_EMPTY;
            while (mqtt_stream_next(&c->in, &header, &body))
            {
                if(header.type != MQTT_PINGRESP)
                {
                    fprintf(stderr, "unexpected packet from the broker: type %d\n", header.type);
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

        if (device_sim_now_ms() >= next_ping_ms)
        {
            if (!ping_answered)
            {
                fprintf(stderr, "no PINGRESP within %ds\n", DEVICE_SIM_KEEP_ALIVE);
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
            next_ping_ms = device_sim_now_ms() + interval_ms;
        }
    }
}

int main(void)
{
    Arena arena = arena_create(device_sim_memory, sizeof(device_sim_memory));

    u8 *rx_buffer = ARENA_PUSH_ARRAY(&arena, u8, DEVICE_SIM_RX_MAX);
    if (!rx_buffer)
    {
        return 1;
    }

    MqttClient client = mqtt_client_create(rx_buffer, DEVICE_SIM_RX_MAX, DEVICE_SIM_RX_PACKET_MAX);
    if (!mqtt_client_connect(&client, DEVICE_SIM_BROKER_IP, DEVICE_SIM_BROKER_PORT, STR_LITERAL(DEVICE_SIM_ID), DEVICE_SIM_KEEP_ALIVE, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("lost")))
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

    printf("press Enter to disconnect\n");
    
    if (!device_sim_wait_for_enter(&client, buffer))
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