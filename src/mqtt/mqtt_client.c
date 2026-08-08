#include "mqtt/mqtt_client.h"
#include "mqtt/mqtt.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>


#define MQTT_CLIENT_CONNECT_MAX 256

b32 mqtt_client_send(MqttClient *c, const MqttWriter *w) 
{
    if (w->overflow)
    {
        fprintf(stderr, "packet does not fit in the buffer\n");
        return 0;
    }

    const ssize_t sent = send(c->sock, w->data, w->length, MSG_NOSIGNAL);
    if (sent != (ssize_t)w->length)
    {
        perror("send");
        return 0;
    }
    return 1;
}

MqttClient mqtt_client_connect(const char *ip, const u16 port, const str client_id, const u16 keep_alive_seconds, const str will_topic, const str will_payload)
{
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    MqttClient client;
    client.sock = sock;
    if (sock < 0)
    {
        perror("socket");
        goto error_handling;
    }

    const int one = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    struct sockaddr_in broker = {0};
    broker.sin_family = AF_INET;
    broker.sin_port = htons(port);
    inet_pton(AF_INET, ip, &broker.sin_addr);

    if (connect(sock, (struct sockaddr *)&broker, sizeof(broker)) < 0)
    {
        perror("connect");
        goto error_handling;
    }

    printf("connected to %s:%d\n", ip, port);

    u8 packet[MQTT_CLIENT_CONNECT_MAX];
    MqttWriter w = mqtt_writer_create(packet, sizeof(packet));
    mqtt_write_connect(&w, client_id, keep_alive_seconds, will_topic, will_payload);
    if (!mqtt_client_send(&client, &w))
    {
        goto error_handling;
    }

    u8 connection_ack[4];
    const size_t recieved = recv(sock, connection_ack, sizeof(connection_ack), 0);
    if(recieved <= 0)
    {
        fprintf(stderr, "the broker closed connection before CONNACK\n");
        goto error_handling;
    }

    MqttReader r = mqtt_reader_create(connection_ack, (u64)recieved);
    const MqttFixedHeader header = mqtt_read_fixed_header(&r);
    const u8 session = mqtt_read_u8(&r);
    const u8 return_code = mqtt_read_u8(&r);
    if (r.error || header.type != MQTT_CONNECT_ACK || header.remaining_length != 2)
    {
        fprintf(stderr, "expected a 4-byte CONNACK, got %zd bytes starting with 0x%02x\n", recieved, connection_ack[0]);
        goto error_handling;
    }
    if (return_code != 0)
    {
        fprintf(stderr, "CONNACK: refused (return code %d)\n", return_code);
        goto error_handling;
    }
    printf("CONNACK: accepted (return code %d, session present %d)\n", return_code, session);
    return client;

    error_handling:
    close(sock);
    client.sock = -1;
    return client;
}