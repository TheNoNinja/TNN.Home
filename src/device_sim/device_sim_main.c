#include "base/base_types.h"
#include "mqtt/mqtt.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEVICE_SIM_BROKER_IP "127.0.0.1"
#define DEVICE_SIM_BROKER_PORT 1883
#define DEVICE_SIM_ID "tnn-sim-1"
#define DEVICE_SIM_TOPIC "homie/5/" DEVICE_SIM_ID "/"

internal b32 device_sim_send(const int sock, const MqttWriter *w)
{
    if (w->overflow)
    {
        fprintf(stderr, "packet does not fit in the buffer\n");
        return 0;
    }

    const ssize_t sent = send(sock, w->data, w->length, 0);
    if (sent != (ssize_t)w->length)
    {
        perror("send");
        return 0;
    }

    return 1;
}

internal b32 device_sim_publish(const int sock, const str topic, const str payload, const b32 retain)
{
    u8 buffer[256];
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
    mqtt_write_publish(&w, topic, payload, retain);
    printf("publish %.*s = %.*s\n", STR_FMT(topic), STR_FMT(payload));
    return device_sim_send(sock, &w);
}

int main(void)
{
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

    u8 buffer[256];
    MqttWriter w = mqtt_writer_create(buffer, sizeof(buffer));
    mqtt_write_connect(&w, STR_LITERAL(DEVICE_SIM_ID), 0);
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
    ok = ok && device_sim_publish(sock, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("init"), 1);
    ok = ok && device_sim_publish(sock, STR_LITERAL(DEVICE_SIM_TOPIC "$description"), STR_LITERAL("{\"homie\":\"5.0\",\"version\":1,\"name\":\"TNN simulator 1\"}"), 1);
    ok = ok && device_sim_publish(sock, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("ready"), 1);

    if (!ok)
    {
        close(sock);
        return 1;
    }

    printf("press Enter to disconnect\n");
    getchar();
    ok = device_sim_publish(sock, STR_LITERAL(DEVICE_SIM_TOPIC "$state"), STR_LITERAL("disconnected"), 1);
    w = mqtt_writer_create(buffer, sizeof(buffer));
    mqtt_write_disconnect(&w);
    ok = ok && device_sim_send(sock, &w);

    close(sock);
    printf("disconnected\n");
    return ok ? 0 : 1;
}