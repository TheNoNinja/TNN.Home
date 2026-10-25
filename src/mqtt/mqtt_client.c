#include "mqtt/mqtt_client.h"

#include <string.h>

MqttClient mqtt_client_create(u8 *in_buffer, const u64 in_capacity, const u64 packet_max, u8 *out_buffer, const u64 out_capacity)
{
    MqttClient c = {0};
    c.connection = mqtt_connection_create(in_buffer, in_capacity, packet_max, out_buffer, out_capacity);
    c.loop = loop_create();
    c.ping_timer = loop_timer_add(&c.loop);
    return c;
}

b32 mqtt_client_connect(MqttClient *c, const char *ip, const u16 port, const str client_id, const u16 keep_alive_seconds, const str will_topic, const str will_payload)
{
    if (!mqtt_connection_connect(&c->connection, ip, port, client_id, keep_alive_seconds, will_topic, will_payload))
    {
        return 0;
    }

    loop_catch_signals();
    loop_watch(&c->loop, c->connection.sock);
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
    if (!loop_wait(&c->loop))
    {
        c->has_failed = c->loop.stop_signal == 0;
        return 0;
    }
    if (loop_timer_fired(&c->loop, c->ping_timer) && !mqtt_connection_ping(&c->connection))
    {
        c->has_failed = 1;
        return 0;
    }
    if (loop_readable(&c->loop, c->connection.sock) && !mqtt_connection_receive(&c->connection))
    {
        c->has_failed = 1;
        return 0;
    }
    return 1;
}

b32 mqtt_client_subscribe(MqttClient *c, const str topic_filter)
{
    return mqtt_connection_subscribe(&c->connection, topic_filter) != 0;
}

b32 mqtt_client_unsubscribe(MqttClient *c, const str topic_filter)
{
    return mqtt_connection_unsubscribe(&c->connection, topic_filter) != 0;
}

b32 mqtt_client_publish(MqttClient *c, const str topic, const str payload, const b32 retain)
{
    return mqtt_connection_publish(&c->connection, topic, payload, retain); 
}

int mqtt_client_close(MqttClient *c, const b32 ok)
{
    if (c->loop.stop_signal != 0)
    {
        LOG_INFO("signal %d (%s): disconnecting", c->loop.stop_signal, strsignal(c->loop.stop_signal));
    }

    if (ok && !c->has_failed)
    {
        return mqtt_connection_disconnect(&c->connection) ? 0 : 1;
    }
    mqtt_connection_close(&c->connection);
    return 1;
}