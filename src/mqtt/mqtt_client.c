#include "base/base_log.h"
#include "mqtt/mqtt_client.h"
#include "mqtt/mqtt.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MQTT_CLIENT_CONNECT_MAX 256

MqttClient mqtt_client_create(u8 *buffer, const u64 capacity, const u64 packet_max)
{
    MqttClient c = {0};
    c.sock = -1;
    c.in = mqtt_stream_create(buffer, capacity, packet_max);
    return c;
}

b32 mqtt_client_send(const MqttClient *c, const MqttWriter *w)
{
    if (w->overflow)
    {
        LOG_ERROR("packet does not fit in the buffer");
        return 0;
    }

    const ssize_t sent = send(c->sock, w->data, w->length, MSG_NOSIGNAL);
    if (sent < 0)
    {
        LOG_ERROR("send: %s", strerror(errno));
        return 0;
    }

    if ((u64)sent != w->length)
    {
        LOG_ERROR("send: only %lld of %llu bytes went out (the send buffer is full)", (long long)sent, (unsigned long long)w->length);
        return 0;
    }
    return 1;
}

b32 mqtt_client_connect(MqttClient *c, const char *ip, const u16 port, const str client_id, const u16 keep_alive_seconds, const str will_topic, const str will_payload)
{
    c->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (c->sock < 0)
    {
        LOG_ERROR("socket: %s", strerror(errno));
        return 0;
    }

    const int one = 1;
    setsockopt(c->sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    struct sockaddr_in broker = {0};
    broker.sin_family = AF_INET;
    broker.sin_port = htons(port);
    inet_pton(AF_INET, ip, &broker.sin_addr);

    if (connect(c->sock, (struct sockaddr *)&broker, sizeof(broker)) < 0)
    {
        LOG_ERROR("connect to %s:%d: %s", ip, port, strerror(errno));
        goto error_handling;
    }

    LOG_INFO("connected to %s:%d", ip, port);

    u8 packet[MQTT_CLIENT_CONNECT_MAX];
    MqttWriter w = mqtt_writer_create(packet, sizeof(packet));
    mqtt_write_connect(&w, client_id, keep_alive_seconds, will_topic, will_payload);
    if (!mqtt_client_send(c, &w))
    {
        goto error_handling;
    }

    MqttFixedHeader header = {0};
    str body = STR_EMPTY;
    while(!mqtt_stream_next(&c->in, &header, &body))
    {
        if (c->in.error || !mqtt_client_receive(c))
        {
            goto error_handling;
        }
    }

    MqttReader r = mqtt_reader_create(body.data, body.length);
    const u8 session = mqtt_read_u8(&r);
    const u8 return_code = mqtt_read_u8(&r);
    if (r.error || header.type != MQTT_CONNECT_ACK || header.remaining_length != 2)
    {
        LOG_ERROR("expected a CONNACK, got packet type %d with %u bytes", header.type, header.remaining_length);
        goto error_handling;
    }
    if (return_code != 0)
    {
        LOG_ERROR("CONNACK: refused (return code %d)", return_code);
        goto error_handling;
    }
    LOG_INFO("CONNACK: accepted (return code %d, session present %d)", return_code, session);
    fcntl(c->sock, F_SETFL, fcntl(c->sock, F_GETFL) | O_NONBLOCK);
    return 1;

error_handling:
    close(c->sock);
    c->sock = -1;
    return 0;
}

b32 mqtt_client_receive(MqttClient *c)
{
    mqtt_stream_compact(&c->in);
    const ssize_t received = recv(c ->sock, c->in.data + c->in.length, c->in.capacity - c->in.length, 0);
    if (received > 0)
    {
        c->in.length += (u64)received;
        return 1;
    }
    if (received == 0)
    {
        LOG_ERROR("the broker closed the connection");
        return 0;
    }

    if (errno == EAGAIN || errno == EINTR)
    {
        return 1;
    }

    LOG_ERROR("recv: %s", strerror(errno));
    return 0;
}