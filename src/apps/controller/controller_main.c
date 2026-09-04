#include "apps/controller/controller.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CONTROLLER_BROKER_IP     "127.0.0.1"
#define CONTROLLER_BROKER_PORT   1883
#define CONTROLLER_CLIENT_ID     "tnn-controller"
#define CONTROLLER_KEEP_ALIVE    10
#define CONTROLLER_TX_MAX        128
#define CONTROLLER_RX_MAX        KB(64)
#define CONTROLLER_RX_PACKET_MAX KB(64)

internal u8 controller_rx_buffer[CONTROLLER_RX_MAX];
internal u8 controller_tx_buffer[CONTROLLER_TX_MAX];

internal b32 controller_receive(Controller *c)\
{
    if (!mqtt_client_receive(&c->client))
    {
        return 0;
    }

    MqttPacket p = {0};
    while(mqtt_client_next(&c->client, &p))
    {
        if (p.type == MQTT_PUBLISH && !controller_on_publish(c, p.publish))
        {
            return 0;
        }
    }
    return !c->client.error;
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
            if (!mqtt_client_ping(&c->client))
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

    u64 port = CONTROLLER_BROKER_PORT;
    if (argc > 1 && (str_to_u64(str_from_cstr(argv[1]), &port) && port >= 1 && port <= 65535))
    {
        LOG_ERROR("usage: %s [port(1 to 65535, default %d)]", argv[0], CONTROLLER_BROKER_PORT);
        return 1;
    }

    Controller c = {0};
    c.client = mqtt_client_create(controller_rx_buffer, sizeof(controller_rx_buffer), CONTROLLER_RX_PACKET_MAX, controller_tx_buffer, sizeof(controller_tx_buffer));
    if (!mqtt_client_connect(&c.client, CONTROLLER_BROKER_IP, (u16)port, STR(CONTROLLER_CLIENT_ID), CONTROLLER_KEEP_ALIVE, STR_EMPTY, STR_EMPTY)){
        return 1;
    }

    b32 ok = mqtt_client_subscribe(&c.client, STR(CONTROLLER_STATE_FILTER));
    
    c.loop = loop_create();
    loop_catch_signals();
    ok = ok && controller_run(&c);

    if (ok)
    {
        return mqtt_client_disconnect(&c.client) ? 0 : 1;
    }
    mqtt_client_close(&c.client);
    return 1;
}