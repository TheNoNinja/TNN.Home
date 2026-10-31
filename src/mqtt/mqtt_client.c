#include "mqtt/mqtt_client.h"

MqttClient mqtt_client_create(u8 *in_buffer, const u64 in_capacity, const u64 packet_max, u8 *out_buffer, const u64 out_capacity)
{
    MqttClient c = {0};
    c.connection = mqtt_connection_create(in_buffer, in_capacity, packet_max, out_buffer, out_capacity);
    c.loop = loop_create();
    c.ping_timer = loop_timer_add(&c.loop);
    return c;
}

internal b32 mqtt_client_send(MqttClient *c)
{
    const str out = mqtt_connection_output(&c->connection);
    if (out.length == 0)
    {
        return 1;
    }
    const i64 sent = platform_send(c->sock, out);
    if (sent < 0)
    {
        LOG_ERROR("send: %s", platform_error());
        return 0;
    }
    mqtt_connection_sent(&c->connection, (u64)sent);

    if ((u64)sent != out.length)
    {
        LOG_ERROR("send: only %lld of %llu bytes went out (the send buffer is full)", (long long)sent, (unsigned long long)out.length);
        return 0;
    }
    return 1;
}

internal b32 mqtt_client_receive(MqttClient *c)
{
    const str space = mqtt_framer_space(&c->connection.in);
    const i64 received = platform_receive(c->sock, space);
    if (received > 0)
    {
        mqtt_framer_received(&c->connection.in, received);
        return 1;
    }
    if (received == 0)
    {
        LOG_ERROR("the broker closed the connection");
        return 0;
    }

    if (received == PLATFORM_NOTHING)
    {
        return 1;
    }

    LOG_ERROR("recv: %s", platform_error());
    return 0;
}

internal b32 mqtt_client_open(MqttClient *c, const char *ip, const u16 port, const str client_id, const u16 keep_alive_seconds, const str will_topic, const str will_payload)
{
    if (!platform_tcp_connect(ip, port, &c->sock))
    {
        LOG_ERROR("socket: %s", platform_error());
        return 0;
    }

    LOG_INFO("connected to %s:%d", ip, port);

    if (!mqtt_connection_connect(&c->connection, client_id, keep_alive_seconds, will_topic, will_payload) || !mqtt_client_send(c))
    {
        goto error_handling;
    }
    PlatformWatch watch = { .handle = c->sock };

    MqttPacket packet = {0};
    while(!mqtt_connection_next(&c->connection, &packet))
    {
        if (c->connection.has_error || !platform_wait(&watch, 1, -1) || !mqtt_client_receive(c))
        {
            goto error_handling;
        }
    }
    return 1;

    error_handling:
    platform_close(&c->sock);
    return 0;
}

b32 mqtt_client_connect(MqttClient *c, const char *ip, const u16 port, const str client_id, const u16 keep_alive_seconds, const str will_topic, const str will_payload)
{
    if (!mqtt_client_open(c, ip, port, client_id, keep_alive_seconds, will_topic, will_payload))
    {
        return 0;
    }

    platform_signals_catch();
    loop_watch(&c->loop, c->sock);
    loop_timer_repeat(&c->loop, c->ping_timer, (u64)keep_alive_seconds * TIME_MS_PER_S);
    return 1;
}

b32 mqtt_client_wait(MqttClient *c)
{
    if (c->connection.has_error)
    {
        c->has_failed = 1;
        return 0;
    }
    if (!mqtt_client_send(c))
    {
        c->has_failed = 1;
        return 0;
    }
    if (!loop_wait(&c->loop))
    {
        c->has_failed = c->loop.stop_signal == 0;
        return 0;
    }
    if (loop_has_timer_fired(&c->loop, c->ping_timer) && !(mqtt_connection_ping(&c->connection) && mqtt_client_send(c)))
    {
        c->has_failed = 1;
        return 0;
    }
    if (loop_is_readable(&c->loop, c->sock) && !mqtt_client_receive(c))
    {
        c->has_failed = 1;
        return 0;
    }
    return 1;
}

b32 mqtt_client_subscribe(MqttClient *c, const str topic_filter)
{
    return mqtt_connection_subscribe(&c->connection, topic_filter) != 0 && mqtt_client_send(c);
}

b32 mqtt_client_unsubscribe(MqttClient *c, const str topic_filter)
{
    return mqtt_connection_unsubscribe(&c->connection, topic_filter) != 0 && mqtt_client_send(c);
}

b32 mqtt_client_publish(MqttClient *c, const str topic, const str payload, const b32 retain)
{
    return mqtt_connection_publish(&c->connection, topic, payload, retain) && mqtt_client_send(c);
}

int mqtt_client_close(MqttClient *c, const b32 ok)
{
    if (c->loop.stop_signal != 0)
    {
        LOG_INFO("signal %d (%s): disconnecting", c->loop.stop_signal, platform_signal_name(c->loop.stop_signal));
    }

    int exit_code = 1;
    if (ok && !c->has_failed && mqtt_connection_disconnect(&c->connection) && mqtt_client_send(c))
    {
        exit_code = 0;
    }
    platform_close(&c->sock);
    return exit_code;
}