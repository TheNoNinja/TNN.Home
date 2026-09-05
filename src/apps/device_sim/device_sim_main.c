#include "base/base.h"
#include "json/json.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"

#include <stdio.h>
#include <string.h>
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

internal b32 device_sim_publish(const MqttClient *c, const str topic, const str payload)
{
    printf("publish %.*s = %.*s\n", STR_FMT(topic), STR_FMT(payload));
    return mqtt_client_publish(c, topic, payload, 1);
}

internal b32 device_sim_light_publish(const MqttClient *c, const str value)
{
    return device_sim_publish(c, STR(DEVICE_SIM_LIGHT_TOPIC "/$target"), value) && device_sim_publish(c, STR(DEVICE_SIM_LIGHT_TOPIC), value);
}

internal void device_sim_write_description(JsonWriter *w)
{
    json_write_object_begin(w);
    json_write_key(w, STR("homie"));
    json_write_str(w, STR("5.0"));
    json_write_key(w, STR("version"));
    json_write_i64(w, DEVICE_SIM_DESCRIPTION_VERSION);
    json_write_key(w, STR("name"));
    json_write_str(w, STR("TNN virtual light"));
    json_write_key(w, STR("nodes"));
    json_write_object_begin(w);
    json_write_key(w, STR("light"));
    json_write_object_begin(w);
    json_write_key(w, STR("name"));
    json_write_str(w, STR("Light"));
    json_write_key(w, STR("properties"));
    json_write_object_begin(w);
    json_write_key(w, STR("on"));
    json_write_object_begin(w);
    json_write_key(w, STR("name"));
    json_write_str(w, STR("On"));
    json_write_key(w, STR("datatype"));
    json_write_str(w, STR("boolean"));
    json_write_key(w, STR("settable"));
    json_write_bool(w, 1);
    json_write_object_end(w);
    json_write_object_end(w);
    json_write_object_end(w);
    json_write_object_end(w);
    json_write_object_end(w);
}

internal b32 device_sim_publish_description(Arena *arena, const MqttClient *c)
{
    const u64 mark = arena->position;

    u8 *buffer = ARENA_PUSH_ARRAY(arena, u8, DEVICE_SIM_DESCRIPTION_MAX);
    if (!buffer)
    {
        return 0;
    }

    JsonWriter w = json_writer_create(buffer, DEVICE_SIM_DESCRIPTION_MAX);
    device_sim_write_description(&w);
    if (w.has_overflown)
    {
        LOG_ERROR("the $description does not fit in %d bytes", DEVICE_SIM_DESCRIPTION_MAX);
    }
    const str description = { w.data, w.length };
    const b32 ok = !w.has_overflown && device_sim_publish(c, STR(DEVICE_SIM_TOPIC "$description"), description);
    arena_pop_to(arena, mark);
    return ok;
    
}

internal b32 device_sim_set(const MqttClient *c, const MqttPublish m)
{
    if (!str_equal(m.topic, STR(DEVICE_SIM_LIGHT_SET_TOPIC)))
    {
        LOG_WARN("a message on %.*s,  which is not our /set topic, ignored", STR_FMT(m.topic));
        return 1;
    }
    if (!str_equal(m.payload, STR("true")) && !str_equal(m.payload, STR("false")))
    {
        LOG_WARN("%s: '%.*s' is not a boolean (true or false), ignored", DEVICE_SIM_LIGHT_SET_TOPIC, STR_FMT(m.payload));
        return 1;
    }
    return device_sim_light_publish(c, m.payload);
}

internal b32 device_sim_handle(const MqttClient *c, const MqttPacket *p)
{
    switch (p->type)
    {
        case MQTT_SUBSCRIBE_ACK:
            return device_sim_publish(c, STR(DEVICE_SIM_TOPIC "$state"), STR("ready"));
        case MQTT_PINGRESP:
            printf("PINGRESP\n");
            return 1;
        case MQTT_PUBLISH:
            printf("received %.*s = %.*s\n", STR_FMT(p->publish.topic), STR_FMT(p->publish.payload));
            return device_sim_set(c, p->publish);
        default:
            return 0;
    }
}

internal b32 device_sim_run(Loop *l, MqttClient *c)
{
    const u64 interval_ms = DEVICE_SIM_KEEP_ALIVE * 1000;
    const u32 ping = loop_timer_add(l);
    loop_timer_start(l, ping, interval_ms);

    loop_watch(l, STDIN_FILENO);
    loop_watch(l, c->sock);

    while(loop_wait(l))
    {
        if (loop_readable(l, STDIN_FILENO))
        {
            return 1;
        }

        if (loop_readable(l, c->sock))
        {
            if (!mqtt_client_receive(c))
            {
                return 0;
            }

            MqttPacket p = {0};
            while (mqtt_client_next(c, &p))
            {
                if (!device_sim_handle(c, &p))
                {
                    return 0;
                }
            }
            if(c->has_error)
            {
                return 0;
            }          
        }

        if (loop_timer_fired(l, ping))
        {
            if (!mqtt_client_ping(c))
            {
                return 0;
            }
            printf("PINGREQ\n");
            loop_timer_start(l, ping, interval_ms);
        }
    }

    if (l->stop_signal == 0)
    {
        return 0;
    }
    LOG_INFO("signal %d (%s): disconnecting", l->stop_signal, strsignal(l->stop_signal));
    return 1;
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
    if(argc > 1 && !(str_to_u64(str_from_cstr(argv[1]), &port) && port >= 1 && port <= 65535))
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
    ok = ok && device_sim_publish(&client, STR(DEVICE_SIM_TOPIC "$state"), STR("init"));
    ok = ok && device_sim_publish_description(&arena, &client);
    ok = ok && device_sim_light_publish(&client, STR("false"));
    ok = ok && mqtt_client_subscribe(&client, STR(DEVICE_SIM_LIGHT_SET_TOPIC)) != 0;
    if (!ok)
    {
        goto error_handling;
    }

    printf("press Enter or Ctrl+C to disconnect\n");
    Loop loop = loop_create();
    loop_catch_signals();
    if (!device_sim_run(&loop, &client))
    {
        goto error_handling;
    }

    if (!device_sim_publish(&client, STR(DEVICE_SIM_TOPIC "$state"), STR("disconnected")) || !mqtt_client_disconnect(&client))
    {
        goto error_handling;
    }
    printf("disconnected\n");
    return 0;

error_handling:
    mqtt_client_close(&client);
    return 1;
}