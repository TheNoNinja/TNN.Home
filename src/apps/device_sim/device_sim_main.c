#include "base/base.h"
#include "homie/homie.h"
#include "json/json.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"

#include <stdio.h>
#include <unistd.h>

#define DEVICE_SIM_BROKER_IP           "127.0.0.1"
#define DEVICE_SIM_BROKER_PORT         1883
#define DEVICE_SIM_ID                  "tnn-sim-1"
#define DEVICE_SIM_TOPIC               "homie/5/" DEVICE_SIM_ID "/"
#define DEVICE_SIM_LIGHT_TOPIC         DEVICE_SIM_TOPIC "light/on"
#define DEVICE_SIM_LIGHT_SET_TOPIC     DEVICE_SIM_LIGHT_TOPIC "/set"
#define DEVICE_SIM_KEEP_ALIVE          10
#define DEVICE_SIM_PACKET_MAX          512
#define DEVICE_SIM_DESCRIPTION_MAX     384
#define DEVICE_SIM_DESCRIPTION_VERSION 2
#define DEVICE_SIM_RX_MAX              KB(8)
#define DEVICE_SIM_RX_PACKET_MAX       KB(4)


internal u8 device_sim_memory[KB(16)];

internal b32 device_sim_publish(const MqttConnection *c, const str topic, const str payload)
{
    printf("publish %.*s = %.*s\n", STR_FMT(topic), STR_FMT(payload));
    return mqtt_connection_publish(c, topic, payload, 1);
}

internal b32 device_sim_light_publish(const MqttConnection *c, const str value)
{
    return device_sim_publish(c, STR(DEVICE_SIM_LIGHT_TOPIC "/$target"), value) && device_sim_publish(c, STR(DEVICE_SIM_LIGHT_TOPIC), value);
}

internal void device_sim_write_description(JsonWriter *w)
{
    json_write_object_begin(w, STR_EMPTY);
        json_write_str(w, STR("homie"), STR("5.0"));
        json_write_i64(w, STR("version"), DEVICE_SIM_DESCRIPTION_VERSION);
        json_write_str(w, STR("name"), STR("TNN virtual light"));

        json_write_object_begin(w, STR("nodes"));

            json_write_object_begin(w, STR("light"));
                json_write_str(w, STR("name"), STR("Light"));
                json_write_object_begin(w, STR("properties"));

                    json_write_object_begin(w, STR("on"));
                        json_write_str(w, STR("name"), STR("On"));
                        json_write_str(w, STR("datatype"), STR("boolean"));
                        json_write_bool(w, STR("settable"), 1);
                    json_write_object_end(w);
                    
                json_write_object_end(w);
            json_write_object_end(w);

        json_write_object_end(w);

    json_write_object_end(w);
}

internal b32 device_sim_publish_description(Arena *arena, const MqttConnection *c)
{
    const u64 mark = arena->position;

    u8 *buffer = ARENA_PUSH_ARRAY(arena, u8, DEVICE_SIM_DESCRIPTION_MAX);
    if (!buffer)
    {
        return 0;
    }

    JsonWriter w = json_writer_create(buffer, DEVICE_SIM_DESCRIPTION_MAX);
    device_sim_write_description(&w);
    if (w.out.has_overflown)
    {
        LOG_ERROR("the $description does not fit in %d bytes", DEVICE_SIM_DESCRIPTION_MAX);
    }
    const b32 ok = !w.out.has_overflown && device_sim_publish(c, STR(DEVICE_SIM_TOPIC "$description"), writer_to_str(&w.out));
    arena_pop_to(arena, mark);
    return ok;
    
}

internal b32 device_sim_set(const MqttConnection *c, const MqttPublish m)
{
    if (!str_equal(m.topic, STR(DEVICE_SIM_LIGHT_SET_TOPIC)))
    {
        LOG_WARN("a message on %.*s,  which is not our /set topic, ignored", STR_FMT(m.topic));
        return 1;
    }
    if (!homie_value_valid(HOMIE_DATATYPE_BOOLEAN, m.payload))
    {
        LOG_WARN("%s: '%.*s' is not a boolean (true or false), ignored", DEVICE_SIM_LIGHT_SET_TOPIC, STR_FMT(m.payload));
        return 1;
    }
    return device_sim_light_publish(c, m.payload);
}

internal b32 device_sim_handle(const MqttClient *c, const MqttPacket *p, const u16 set_subscription)
{
    switch (p->type)
    {
        case MQTT_SUBACK:
            if (p->packet_id != set_subscription)
            {
                LOG_WARN("a SUBACK for packet id %d, not out /set, ignored", p->packet_id);
                return 1;
            }
            return device_sim_publish(&c->connection, STR(DEVICE_SIM_TOPIC "$state"), STR("ready"));
        case MQTT_PINGRESP:
            printf("PINGRESP\n");
            return 1;
        case MQTT_PUBLISH:
            printf("received %.*s = %.*s\n", STR_FMT(p->publish.topic), STR_FMT(p->publish.payload));
            return device_sim_set(&c->connection, p->publish);
        default:
            return 0;
    }
}

int main(const int argc, const char **argv)
{
    setvbuf(stdout, 0, _IOLBF, 0);

    Arena arena = arena_create(device_sim_memory, sizeof(device_sim_memory));

    u8 *rx_buffer = ARENA_PUSH_ARRAY(&arena, u8, DEVICE_SIM_RX_MAX);
    u8 *tx_buffer = ARENA_PUSH_ARRAY(&arena, u8, DEVICE_SIM_PACKET_MAX);
    if (!rx_buffer || !tx_buffer)
    {
        return 1;
    }

    u64 port = DEVICE_SIM_BROKER_PORT;
    if(argc > 1 && !str_to_u64_range(str_from_cstr(argv[1]), 1, 65535, &port))
    {
        LOG_ERROR("usage: %s [port(default '%d')]", argv[0], DEVICE_SIM_BROKER_PORT);
        return 1;
    }

    MqttClient client = mqtt_client_create(rx_buffer, DEVICE_SIM_RX_MAX, DEVICE_SIM_RX_PACKET_MAX, tx_buffer, DEVICE_SIM_PACKET_MAX);
    if (!mqtt_client_connect(&client, DEVICE_SIM_BROKER_IP, (u16)port, STR(DEVICE_SIM_ID), DEVICE_SIM_KEEP_ALIVE, STR(DEVICE_SIM_TOPIC "$state"), STR("lost")))
    {
        return 1;
    }

    b32 ok = 1;
    ok = ok && device_sim_publish(&client.connection, STR(DEVICE_SIM_TOPIC "$state"), STR("init"));
    ok = ok && device_sim_publish_description(&arena, &client.connection);
    ok = ok && device_sim_light_publish(&client.connection, STR("false"));
    const u16 set_subscription = mqtt_connection_subscribe(&client.connection, STR(DEVICE_SIM_LIGHT_SET_TOPIC));
    ok = ok && set_subscription != 0;

    printf("press Enter or Ctrl+C to disconnect\n");
    loop_watch(&client.loop, STDIN_FILENO);
    while (ok && mqtt_client_wait(&client))
    {
        if (loop_readable(&client.loop, STDIN_FILENO))
        {
            break;
        }
        if (loop_timer_fired(&client.loop, client.ping_timer))
        {
            printf("PINGREQ\n");
        }
        MqttPacket p = {0};
        while (ok && mqtt_connection_next(&client.connection, &p))
        {
            ok = device_sim_handle(&client, &p, set_subscription);
        }
    }

    ok = ok && !client.has_failed&& device_sim_publish(&client.connection, STR(DEVICE_SIM_TOPIC "$state"), STR("disconnected"));
    const int exit_code = mqtt_client_close(&client, ok);
    if (exit_code == 0)
    {
        printf("disconnected\n");
    }
    return exit_code;
}