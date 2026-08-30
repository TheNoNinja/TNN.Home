#include "base/base.h"
#include "homie/homie.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CONTROLLER_BROKER_IP     "127.0.0.1"
#define CONTROLLER_BROKER_PORT   1883
#define CONTROLLER_CLIENT_ID     "tnn-controller"
#define CONTROLLER_STATE_FILTER  "+/5/+/$state"
#define CONTROLLER_KEEP_ALIVE    10
#define CONTROLLER_TOPIC_MAX     96
#define CONTROLLER_TX_MAX        128
#define CONTROLLER_RX_MAX        KB(64)
#define CONTROLLER_RX_PACKET_MAX KB(64)
#define CONTROLLER_DEVICE_MAX    64
#define CONTROLLER_LINE_MAX      128

internal u8 controller_rx_buffer[CONTROLLER_RX_MAX];

typedef struct ControllerDevice ControllerDevice;
struct ControllerDevice {
    HomieId domain;
    HomieId id;
    HomieState state;
    u64 description_length;
};

typedef struct Controller Controller;
struct Controller
{
    MqttClient client;
    Loop loop;
    u16 packet_id;
    ControllerDevice devices[CONTROLLER_DEVICE_MAX];
    u8 line[CONTROLLER_LINE_MAX];
    u64 line_length;
};

internal b32 controller_subscribe(Controller *c, const str filter)
{
    c->packet_id += 1;
    if (c->packet_id == 0)
    {
        c->packet_id = 1;
    }
    u8 packet[CONTROLLER_TX_MAX];
    MqttWriter w = mqtt_writer_create(packet, sizeof(packet));
    mqtt_write_subscribe(&w, c->packet_id, filter);
    LOG_INFO("SUBSCRIBE %.*s (packet id %d)", STR_FMT(filter), c->packet_id);
    return mqtt_client_send(&c->client, &w);
}

internal ControllerDevice *controller_find(Controller *c, const HomieTopic *t)
{
    for (u32 i = 0; i < CONTROLLER_DEVICE_MAX; i++)
    {
        ControllerDevice *d = &c->devices[i];
        if (d->id.length > 0 && str_equal(homie_id_str(&d->id), t->device) && str_equal(homie_id_str(&d->domain), t->domain))
        {
            return d;
        }
    }
    return 0;
}

internal ControllerDevice *controller_add(Controller *c, const HomieTopic *t)
{
    for (u32 i = 0; i < CONTROLLER_DEVICE_MAX; i++)
    {
        ControllerDevice *d = &c->devices[i];
        if (d->id.length == 0)
        {
            d->domain = homie_id_copy(t->domain);
            d->id = homie_id_copy(t->device);
            return d;
        }
    }
    LOG_WARN("the device table is full (%d devices): %.*s ignored", CONTROLLER_DEVICE_MAX, STR_FMT(t->device));
    return 0;
}

internal b32 controller_subscribe_description(Controller *c, const ControllerDevice *d)
{
    const str domain = homie_id_str(&d->domain);
    const str id = homie_id_str(&d->id);
    u8 buffer[CONTROLLER_TOPIC_MAX];
    const str filter = homie_topic_build(buffer, sizeof(buffer), domain, id, STR("$description"));
    ASSERT(filter.length > 0);
    return controller_subscribe(c, filter);
}

internal b32 controller_on_state(Controller *c, const HomieTopic *t, const str payload)
{
    if (payload.length == 0)
    {
        ControllerDevice *d = controller_find(c, t);
        if (d)
        {
            printf("%.*s: removed\n", STR_FMT(t->device));
            *d = (ControllerDevice){0};
        }
        return 1;
    }

    const HomieState state = homie_state_parse(payload);
    if (state == HOMIE_STATE_NONE)
    {
        LOG_WARN("%.*s: '%.*s' is not a Homie $state, ignored", STR_FMT(t->device), STR_FMT(payload));
        return 1;
    }
    ControllerDevice *d = controller_find(c, t);
    if (d)
    {
        if (d->state == state)
        {
            return 1;
        }
        printf("%.*s: %.*s -> %.*s\n", STR_FMT(t->device), STR_FMT(homie_state_name(d->state)), STR_FMT(payload));
        d->state = state;
    }
    else 
    {
        d = controller_add(c, t);
        if (!d)
        {
            return 1;
        }
        printf("%.*s: new device, %.*s\n", STR_FMT(t->device), STR_FMT(payload));
        d->state = state;
        if(!controller_subscribe_description(c, d))
        {
            return 0;
        }
    }

    // This fixes a problem with NanoMQ
    // Im considering using mosquitto exclusively and not supporting NanoMQ altogether
    if (state == HOMIE_STATE_INIT)
    {
        return controller_subscribe(c, STR(CONTROLLER_STATE_FILTER));
    }
    return 1;
}

internal void controller_on_description(Controller *c, const HomieTopic *t, const str payload)
{
    ControllerDevice *d = controller_find(c, t);
    if (d)
    {
        d->description_length = payload.length;
        printf("%.*s: $description, %llu bytes\n", STR_FMT(t->device), (unsigned long long)payload.length);
    }
}

internal b32 controller_on_publish(Controller *c, const MqttPublish m)
{
    HomieTopic t = {0};
    if (!homie_topic_parse(m.topic, &t))
    {
        LOG_WARN("%.*s is not a Homie v5 device topic, ignored", STR_FMT(m.topic));
        return 1;
    }
    if(str_equal(t.rest, STR("$state")))
    {
        return controller_on_state(c, &t, m.payload);
    }
    if (str_equal(t.rest, STR("$description")))
    {
        controller_on_description(c, &t, m.payload);
        return 1;
    }
    LOG_WARN("%.*s: a topic we did not subscribe to, ignored", STR_FMT(m.topic));
    return 1;
}

internal b32 controller_handle(Controller *c, const MqttFixedHeader header, const str body)
{
    MqttReader r = mqtt_reader_create(body.data, body.length);

    switch(header.type)
    {
        case MQTT_SUBSCRIBE_ACK:
        {
            const u16 packet_id = mqtt_read_u16_be(&r);
            const u8 return_code = mqtt_read_u8(&r);
            if (r.error || return_code != MQTT_SUBSCRIBE_ACK_QOS_0)
            {
                LOG_ERROR("SUBACK: refused (packet id %d, return code 0x%02x)", packet_id, return_code);
                return 0;
            }
            LOG_INFO("SUBACK (packet id %d)", packet_id);
            return 1;
        }
        case MQTT_PUBLISH:
        {
            const MqttPublish m = mqtt_read_publish(&r, header.flags);
            if (r.error)
            {
                LOG_ERROR("malformed PUBLISH of %llu bytes", (unsigned long long) body.length);
                return 0;
            }
            return controller_on_publish(c, m);
        }
        case MQTT_PINGRESP:
        {
            return 1;
        }
        default:
        {
            LOG_WARN("unexpected packet type %d, ignored", header.type);
            return 1;
        }
    }
}

internal b32 controller_receive(Controller *c)\
{
    if (!mqtt_client_receive(&c->client))
    {
        return 0;
    }
    MqttFixedHeader header = {0};
    str body = STR_EMPTY;
    while(mqtt_stream_next(&c->client.in, &header, &body))
    {
        if (!controller_handle(c, header, body))
        {
            return 0;
        }
    }
    return !c->client.in.error;
}

internal void controller_list(const Controller *c)
{
    u32 count = 0;
    for (u32 i = 0; i < CONTROLLER_DEVICE_MAX; i++)
    {
        const ControllerDevice *d = &c->devices[i];
        if (d->id.length == 0){
            continue;
        }
        const str id = homie_id_str(&d->id);
        const str domain = homie_id_str(&d->domain);
        const str state = homie_state_name(d->state);
        printf("%.*s %.*s (domain %.*s, $description %llu bytes)\n", STR_FMT(id), STR_FMT(state), STR_FMT(domain), (unsigned long long)d->description_length);
        count += 1;
    }
    printf("%u device%s\n", count, count == 1 ? "" : "s");
}

internal void controller_command(const Controller *c, const str line)
{
    if (str_equal(line, STR("list")))
    {
        controller_list(c);
    }
    else if (line.length > 0)
    {
        printf("unknown command '%.*s'; the command is: list\n", STR_FMT(line));
    }
}

internal b32 controller_read_input(Controller *c)
{
    u8 bytes[CONTROLLER_LINE_MAX];
    const ssize_t count = read(STDIN_FILENO, bytes, sizeof(bytes));
    if (count <= 0)
    {
        return 0;
    }
    for (u64 i = 0; i < (u64)count; i++)
    {
        if(bytes[i] == '\n')
        {
            controller_command(c, (str){ c->line, c->line_length});
            c->line_length = 0;
        }
        else if (c->line_length < CONTROLLER_LINE_MAX)
        {
            c->line[c->line_length] = bytes[i];
            c->line_length += 1;
        }
    }
    return 1;
}

internal b32 controller_run(Controller *c)
{
    const u32 ping = loop_timer_add(&c->loop);
    loop_timer_start(&c->loop, ping, CONTROLLER_KEEP_ALIVE * 1000);
    loop_watch(&c->loop, STDIN_FILENO);
    loop_watch(&c->loop, c->client.sock);
    

    while(loop_wait(&c->loop))
    {
        if (loop_readable(&c->loop, STDIN_FILENO) && !controller_read_input(c))
        {
            LOG_INFO("stdin closed: disconnecting");
            return 1;
        }
        if (loop_readable(&c->loop, c->client.sock) && !controller_receive(c))
        {
            return 0;
        }
        if (loop_timer_fired(&c->loop, ping))
        {
            u8 packet[CONTROLLER_TX_MAX];
            MqttWriter w = mqtt_writer_create(packet, sizeof(packet));
            mqtt_write_pingreq(&w);
            if (!mqtt_client_send(&c->client, &w))
            {
                return 0;
            }
            loop_timer_start(&c->loop, ping, CONTROLLER_KEEP_ALIVE * 1000);
        }
    }

    if (c->loop.stop_signal == 0)
    {
        return 0;
    }
    LOG_INFO("signal %d (%s): disconnecting", c->loop.stop_signal, strsignal(c->loop.stop_signal));
    return 1;
}

int main(const int argc, char **argv)
{
    setvbuf(stdout, 0, _IOLBF, 0);

    u16 port = CONTROLLER_BROKER_PORT;
    if (argc > 1)
    {
        char *end = 0;
        const long value = strtol(argv[1], &end, 10);
        if (*end != '\0' || value < 1 || value > 65535)
        {
            LOG_ERROR("usage: %s [port(1 to 65535, default %d)]", argv[0], CONTROLLER_BROKER_PORT);
            return 1;
        }
        port = (u16)value;
    }

    Controller c = {0};
    c.client = mqtt_client_create(controller_rx_buffer, sizeof(controller_rx_buffer), CONTROLLER_RX_PACKET_MAX);
    if (!mqtt_client_connect(&c.client, CONTROLLER_BROKER_IP, port, STR(CONTROLLER_CLIENT_ID), CONTROLLER_KEEP_ALIVE, STR_EMPTY, STR_EMPTY)){
        return 1;
    }

    b32 ok = controller_subscribe(&c, STR(CONTROLLER_STATE_FILTER));
    
    c.loop = loop_create();
    loop_catch_signals();
    ok = ok && controller_run(&c);

    if (ok)
    {
        u8 packet[CONTROLLER_TX_MAX];
        MqttWriter w = mqtt_writer_create(packet, sizeof(packet));
        mqtt_write_disconnect(&w);
        ok = mqtt_client_send(&c.client, &w);
    }
    close(c.client.sock);
    return ok ? 0 : 1;
}