#include "apps/controller/controller.h"

#include <stdio.h>
#include <unistd.h>

#define CONTROLLER_BROKER_IP     "127.0.0.1"
#define CONTROLLER_BROKER_PORT   1883
#define CONTROLLER_HTTP_IP       "127.0.0.1"
#define CONTROLLER_HTTP_PORT     8080
#define CONTROLLER_CLIENT_ID     "tnn-controller"
#define CONTROLLER_KEEP_ALIVE    10
#define CONTROLLER_TX_MAX        256
#define CONTROLLER_RX_MAX        KB(64)
#define CONTROLLER_RX_PACKET_MAX KB(64)
#define CONTROLLER_HEARTBEAT_MS  15000

internal u8 controller_rx_buffer[CONTROLLER_RX_MAX];
internal u8 controller_tx_buffer[CONTROLLER_TX_MAX];

internal Controller controller;

internal b32 controller_receive(Controller *c)
{
    MqttPacket p = {0};
    while(mqtt_connection_next(&c->client.connection, &p))
    {
        if (p.type == MQTT_PUBLISH && !controller_on_publish(c, p.publish))
        {
            return 0;
        }
    }
    return 1;
}

internal b32 controller_run(Controller *c)
{
    const u32 heartbeat = loop_timer_add(&c->client.loop);
    loop_timer_repeat(&c->client.loop, heartbeat, CONTROLLER_HEARTBEAT_MS);
    loop_watch(&c->client.loop, STDIN_FILENO);

    while(mqtt_client_wait(&c->client))
    {
        if (loop_readable(&c->client.loop, STDIN_FILENO))
        {
            b32 is_closed = 0;
            if (!controller_read_input(c, &is_closed))
            {
                return 0;
            }
            if (is_closed)
            {
                LOG_INFO("stdin closed: disconnecting");
                return 1;
            }
        }
        
        if (!controller_receive(c))
        {
            return 0;
        }

        if (!controller_http_handle(c))
        {
            return 0;
        }

        if (loop_timer_fired(&c->client.loop, heartbeat))
        {
            controller_events_heartbeat(c);
        }
    }

    return 1;
}

int main(const int argc, char **argv)
{
    setvbuf(stdout, 0, _IOLBF, 0);

    u64 broker_port = CONTROLLER_BROKER_PORT;
    u64 http_port = CONTROLLER_HTTP_PORT;
    if ((argc > 1 && !cstr_to_u64_range(argv[1], 1, 65535, &broker_port)) || (argc > 2 && !cstr_to_u64_range(argv[2], 1, 65535, &http_port)) || argc > 3)
    {
        LOG_ERROR("usage: %s [broker port(1 to 65535, default %d)] [http port(1 to 65535, default %d)]", argv[0], CONTROLLER_BROKER_PORT, CONTROLLER_HTTP_PORT);
        return 1;
    }

    Controller *c = &controller;
    c->client = mqtt_client_create(controller_rx_buffer, sizeof(controller_rx_buffer), CONTROLLER_RX_PACKET_MAX, controller_tx_buffer, sizeof(controller_tx_buffer));
    if (!mqtt_client_connect(&c->client, CONTROLLER_BROKER_IP, (u16)broker_port, STR(CONTROLLER_CLIENT_ID), CONTROLLER_KEEP_ALIVE, STR_EMPTY, STR_EMPTY))
    {
        return 1;
    }

    b32 ok = mqtt_client_subscribe(&c->client, STR(HOMIE_DISCOVERY_FILTER));
    ok = ok && controller_http_listen(c, CONTROLLER_HTTP_IP, (u16)http_port);
    ok = ok && controller_run(c);

    return mqtt_client_close(&c->client, ok);
}