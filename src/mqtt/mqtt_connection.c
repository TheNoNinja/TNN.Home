#include "mqtt/mqtt_connection.h"
#include "mqtt/mqtt_encode.h"
#include "mqtt/mqtt_protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

MqttConnection mqtt_connection_create(u8 *in_buffer, const u64 in_capacity, const u64 packet_max, u8 *out_buffer, const u64 out_capacity)
{
    MqttConnection c = {0};
    c.sock = -1;
    c.in = mqtt_framer_create(in_buffer, in_capacity, packet_max);
    c.out = out_buffer;
    c.out_capacity = out_capacity;
    return c;
}

internal b32 mqtt_connection_send(const MqttConnection *c, const Writer *w)
{
    if (w->has_overflown)
    {
        LOG_ERROR("packet does not fit in the send buffer (%llu bytes)", (unsigned long long)w->capacity);
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

b32 mqtt_connection_connect(MqttConnection *c, const char *ip, const u16 port, const str client_id, const u16 keep_alive_seconds, const str will_topic, const str will_payload)
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

    Writer w = writer_create(c->out, c->out_capacity);
    mqtt_write_connect(&w, client_id, keep_alive_seconds, will_topic, will_payload);
    if (!mqtt_connection_send(c, &w))
    {
        goto error_handling;
    }

    MqttFixedHeader header = {0};
    str body = STR_EMPTY;
    while(!mqtt_framer_next(&c->in, &header, &body))
    {
        if (c->in.has_error || !mqtt_connection_receive(c))
        {
            goto error_handling;
        }
    }

    MqttPacket packet = {0};
    if (!mqtt_packet_parse(header, body, &packet) || packet.type != MQTT_CONNACK)
    {
        LOG_ERROR("expected a CONNACK, got packet type %d with %u bytes", header.type, header.remaining_length);
        goto error_handling;
    }
    if (packet.return_code != 0)
    {
        LOG_ERROR("CONNACK: refused (return code %d)", packet.return_code);
        goto error_handling;
    }
    LOG_INFO("CONNACK: accepted (return code %d)", packet.return_code);
    fcntl(c->sock, F_SETFL, fcntl(c->sock, F_GETFL) | O_NONBLOCK);
    return 1;

error_handling:
    mqtt_connection_close(c);
    return 0;
}

b32 mqtt_connection_receive(MqttConnection *c)
{
    const str space = mqtt_framer_space(&c->in);
    const ssize_t received = recv(c ->sock, space.data, space.length, 0);
    if (received > 0)
    {
        mqtt_framer_received(&c->in, received);
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

b32 mqtt_connection_publish(const MqttConnection *c, const str topic, const str payload, const b32 retain)
{
    Writer w = writer_create(c->out, c->out_capacity);
    mqtt_write_publish(&w, topic, payload, retain);
    return mqtt_connection_send(c, &w);
}

internal u16 mqtt_connection_next_packet_id(MqttConnection *c)
{
    c->packet_id += 1;
    if (c->packet_id == 0)
    {
        c->packet_id = 1;
    }
    return c->packet_id;
}

u16 mqtt_connection_subscribe(MqttConnection *c, const str topic_filter)
{
    const u16 packet_id = mqtt_connection_next_packet_id(c);
    Writer w = writer_create(c->out, c->out_capacity);
    mqtt_write_subscribe(&w, packet_id, topic_filter);
    LOG_INFO("SUBSCRIBE %.*s (packet id %d)", STR_FMT(topic_filter), packet_id);
    return mqtt_connection_send(c, &w) ? packet_id : 0;
}

u16 mqtt_connection_unsubscribe(MqttConnection *c, const str filter)
{
    const u16 packet_id = mqtt_connection_next_packet_id(c);
    Writer w = writer_create(c->out, c->out_capacity);
    mqtt_write_unsubscribe(&w, packet_id, filter);
    LOG_INFO("UNSUBSCRIBE %.*s (packet id %d)", STR_FMT(filter), packet_id);
    return mqtt_connection_send(c, &w) ? packet_id : 0;
}

b32 mqtt_connection_ping(MqttConnection *c)
{
    if (c->is_waiting_on_ping)
    {
        LOG_ERROR("no PINGRESP to the last PINGREQ: the broker did not answer");
        return 0;
    }

    Writer w = writer_create(c->out, c->out_capacity);
    mqtt_write_pingreq(&w);
    c->is_waiting_on_ping = 1;
    return mqtt_connection_send(c, &w);
}

b32 mqtt_connection_disconnect(MqttConnection *c)
{
    Writer w = writer_create(c->out, c->out_capacity);
    mqtt_write_disconnect(&w);
    const b32 ok = mqtt_connection_send(c, &w);
    mqtt_connection_close(c);
    return ok;
}

void mqtt_connection_close(MqttConnection *c)
{
    if (c->sock >= 0)
    {
        close(c->sock);
    }
    c->sock = -1;
}

b32 mqtt_connection_next(MqttConnection *c, MqttPacket *packet)
{
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;
    while (!c->has_error && mqtt_framer_next(&c->in, &header, &body))
    {
        if(!mqtt_packet_parse(header, body, packet))
        {
            LOG_ERROR("malformed packet (type %d, %llu bytes)", header.type, (unsigned long long)body.length);
            c->has_error = 1;
            return 0;
        }

        switch (packet->type)
        {
            case MQTT_PUBLISH: return 1;
            case MQTT_SUBACK:
            {
                if (packet->return_code != MQTT_SUBACK_QOS_0)
                {
                    LOG_ERROR("SUBACK: refused (packet id %d, return code %d)", packet->packet_id, packet->return_code);
                    c->has_error = 1;
                    return 0;
                }
                LOG_INFO("SUBACK (packet id %d)", packet->packet_id);
                return 1;
            }
            case MQTT_UNSUBACK:
            {
                LOG_INFO("UNSUBACK (packet id %d)", packet->packet_id);
                return 1;
            }
            case MQTT_PINGRESP:
            {
                c->is_waiting_on_ping = 0;
                return 1;
            }
            default:
            {
                LOG_WARN("unexpected packet type %d, ignored", packet->type);
                break;
            }
        }
    }

    if (c->in.has_error)
    {
        c->has_error = 1;
    }
    return 0;
}