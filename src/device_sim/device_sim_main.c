#include "base/base_types.h"
#include "base/base_arena.h"
#include "mqtt/mqtt.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define DEVICE_SIM_BROKER_IP   "127.0.0.1"
#define DEVICE_SIM_BROKER_PORT 1883
#define DEVICE_SIM_ID          "tnn-sim-1"
#define DEVICE_SIM_TOPIC       "homie/5/" DEVICE_SIM_ID "/"
#define DEVICE_SIM_KEEP_ALIVE  10
#define DEVICE_SIM_PACKET_MAX  256


internal u8 device_sim_memory[KB(8)];

internal b32 device_sim_send(const int sock, const MqttWriter *w)
{
    if (w->overflow)
    {
        fprintf(stderr, "packet does not fit in the buffer\n");
        return 0;
    }

    const ssize_t sent = send(sock, w->data, w->length, MSG_NOSIGNAL);
    if (sent != (ssize_t)w->length)
    {
        perror("send");
        return 0;
    }

    return 1;
}

internal b32 device_sim_publish(Arena *arena, const int sock, const str topic, const str payload, const b32 retain)
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
    const b32 ok = device_sim_send(sock, &w);
    arena_pop_to(arena, mark);
    return ok;
}

internal u64 device_sim_now_ms(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

internal b32 device_sim_wait_for_enter(const int sock, u8 *buffer)
{
    const u64 interval_ms = DEVICE_SIM_KEEP_ALIVE * 1000;
    u64 next_ping_ms = device_sim_now_ms() + interval_ms;
    b32 ping_answered = 1;

    for(;;)
    {
        struct pollfd fds[2] = {0};
        fds[0].fd = STDIN_FILENO;
        fds[0].events = POLLIN;
        fds[1].fd = sock;
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
            u8 packet[2];
            const ssize_t received = recv(sock, packet, sizeof(packet), 0);
            if (received <= 0)
            {
                fprintf(stderr, "the broker closed the connection\n");
                return 0;
            }
            if (received != 2 || packet[0] != (MQTT_PINGRESP << 4))
            {
                fprintf(stderr, "unexpected packet from the broker: 0x%02x\n", packet[0]);
                return 0;
            }
            printf("PINGRESP\n");
            ping_answered = 1;
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
            if (!device_sim_send(sock, &w))
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

    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    const int one = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    struct sockaddr_in broker = {0};
    broker.sin_family = AF_INET;
    broker.sin_port = htons(DEVICE_SIM_BROKER_PORT);
    inet_pton(AF_INET, DEVICE_SIM_BROKER_IP, &broker.sin_addr);

    if (connect(sock, (struct sockaddr*) &broker, sizeof(broker)) < 0)
    {
        perror("connect");
        close(sock);
        return 1;
    }

    printf("connected to %s:%d\n", DEVICE_SIM_BROKER_IP, DEVICE_SIM_BROKER_PORT);

    u8* buffer = ARENA_PUSH_ARRAY(&arena, u8, DEVICE_SIM_PACKET_MAX);
    if (!buffer){
        close(sock);
        return 1;
    }

    MqttWriter w = mqtt_writer_create(buffer, DEVICE_SIM_PACKET_MAX);
    mqtt_write_connect(&w, STR_LITERAL(DEVICE_SIM_ID), DEVICE_SIM_KEEP_ALIVE, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("lost"));
    if (!device_sim_send(sock, &w))
    {
        close(sock);
        return 1;
    }

    u8 connection_ack[4];
    const ssize_t received = recv(sock, connection_ack, sizeof(connection_ack), 0);
    if (received != 4 || connection_ack[0] != MQTT_CONNECT_ACK << 4)
    {
        fprintf(stderr, "expected a 4-byte CONNACK, got %zd bytes\n", received);
        close(sock);
        return 1;
    }
    if (connection_ack[3] != 0)
    {
        fprintf(stderr, "CONNACK: refused (return code %d)\n", connection_ack[3]);
        close(sock);
        return 1;
    }
    printf("CONNACK: accepted (return code 0)\n");

    b32 ok = 1;
    ok = ok && device_sim_publish(&arena, sock, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("init"), 1);
    ok = ok && device_sim_publish(&arena, sock, STR_LITERAL(DEVICE_SIM_TOPIC "$description"), STR_LITERAL("{\"homie\":\"5.0\",\"version\":1,\"name\":\"TNN simulator 1\"}"), 1);
    ok = ok && device_sim_publish(&arena, sock, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("ready"), 1);

    if (!ok)
    {
        close(sock);
        return 1;
    }

    printf("press Enter to disconnect\n");
    
    if (!device_sim_wait_for_enter(sock, buffer))
    {
        close(sock);
        return 1;
    }

    ok = device_sim_publish(&arena, sock, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("disconnected"), 1);
    w = mqtt_writer_create(buffer, DEVICE_SIM_PACKET_MAX);
    mqtt_write_disconnect(&w);
    ok = ok && device_sim_send(sock, &w);

    close(sock);
    printf("disconnected\n");
    return ok ? 0 : 1;
}