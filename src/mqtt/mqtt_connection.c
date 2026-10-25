#include "mqtt/mqtt_connection.h"
#include "mqtt/mqtt_encode.h"
#include "mqtt/mqtt_protocol.h"

#include <string.h>

MqttConnection mqtt_connection_create(u8 *in_buffer, const u64 in_capacity, const u64 packet_max, u8 *out_buffer, const u64 out_capacity)
{
    MqttConnection c = {0};
    c.in = mqtt_framer_create(in_buffer, in_capacity, packet_max);
    c.out = out_buffer;
    c.out_capacity = out_capacity;
    return c;
}

str mqtt_connection_output(const MqttConnection *c)
{
    return (str){ .data = c->out, .length = c->out_length };
}

void mqtt_connection_sent(MqttConnection *c, const u64 count)
{
    ASSERT(count <= c->out_length);
    memmove(c->out, c->out + count, c->out_length - count);
    c->out_length -= count;
}

internal Writer mqtt_connection_writer(const MqttConnection *c)
{
    ASSERT(c->state != MQTT_CONNECTION_DISCONNECTED);
    return writer_create(c->out + c->out_length, c->out_capacity - c->out_length);
}

internal b32 mqtt_connection_queue(MqttConnection *c, const Writer *w)
{
    if (w->has_overflown)
    {
        LOG_ERROR("packet does not fit in the send buffer (%llu bytes)", (unsigned long long)c->out_capacity);
        return 0;
    }
    c->out_length += w->length;
    return 1;
}

b32 mqtt_connection_connect(MqttConnection *c, const str client_id, const u16 keep_alive_seconds, const str will_topic, const str will_payload)
{
    ASSERT(c->out_length == 0);
    Writer w = mqtt_connection_writer(c);
    mqtt_write_connect(&w, client_id, keep_alive_seconds, will_topic, will_payload);
    c->state = MQTT_CONNECTION_CONNECTING;
    return mqtt_connection_queue(c, &w);
}

b32 mqtt_connection_publish(MqttConnection *c, const str topic, const str payload, const b32 retain)
{
    Writer w = mqtt_connection_writer(c);
    mqtt_write_publish(&w, topic, payload, retain);
    return mqtt_connection_queue(c, &w);
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
    Writer w = mqtt_connection_writer(c);
    mqtt_write_subscribe(&w, packet_id, topic_filter);
    LOG_INFO("SUBSCRIBE %.*s (packet id %d)", STR_FMT(topic_filter), packet_id);
    return mqtt_connection_queue(c, &w) ? packet_id : 0;
}

u16 mqtt_connection_unsubscribe(MqttConnection *c, const str filter)
{
    const u16 packet_id = mqtt_connection_next_packet_id(c);
    Writer w = mqtt_connection_writer(c);
    mqtt_write_unsubscribe(&w, packet_id, filter);
    LOG_INFO("UNSUBSCRIBE %.*s (packet id %d)", STR_FMT(filter), packet_id);
    return mqtt_connection_queue(c, &w) ? packet_id : 0;
}

b32 mqtt_connection_ping(MqttConnection *c)
{
    if (c->is_waiting_on_ping)
    {
        LOG_ERROR("no PINGRESP to the last PINGREQ: the broker did not answer");
        return 0;
    }

    Writer w = mqtt_connection_writer(c);
    mqtt_write_pingreq(&w);
    c->is_waiting_on_ping = 1;
    return mqtt_connection_queue(c, &w);
}

b32 mqtt_connection_disconnect(MqttConnection *c)
{
    Writer w = mqtt_connection_writer(c);
    mqtt_write_disconnect(&w);
    const b32 ok = mqtt_connection_queue(c, &w);
    c->state = MQTT_CONNECTION_DISCONNECTED;
    return ok;
}

internal b32 mqtt_connection_accept(MqttConnection *c, const MqttPacket *packet)
{
    if (packet->type != MQTT_CONNACK)
    {
        LOG_ERROR("expected a CONNACK, got packet type %d", packet->type);
        c->has_error = 1;
        return 0;
    }
    if (packet->return_code != 0)
    {
        LOG_ERROR("CONNACK: refused (return code %d)", packet->return_code);
        c->has_error = 1;
        return 0;
    }
    LOG_INFO("CONNACK: accepted (return code %d)", packet->return_code);
    c->state = MQTT_CONNECTION_CONNECTED;
    return 1;
}

b32 mqtt_connection_next(MqttConnection *c, MqttPacket *packet)
{
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;
    while (!c->has_error && mqtt_framer_next(&c->in, &header, &body))
    {
        if (!mqtt_packet_parse(header, body, packet))
        {
            LOG_ERROR("malformed packet (type %d, %llu bytes)", header.type, (unsigned long long)body.length);
            c->has_error = 1;
            return 0;
        }

        if (c->state == MQTT_CONNECTION_CONNECTING)
        {
            return mqtt_connection_accept(c, packet);
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