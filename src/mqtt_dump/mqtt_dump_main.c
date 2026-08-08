#include "base/base_types.h"
#include "mqtt/mqtt.h"

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define MQTT_DUMP_BROKER_IP "127.0.0.1"
#define MQTT_DUMP_BROKER_PORT 1883
#define MQTT_DUMP_CLIENT_ID "tnn-mqtt-dump"
#define MQTT_DUMP_TOPIC_FILTER "homie/5/#"
#define MQTT_DUMP_KEEP_ALIVE 10
#define MQTT_DUMP_TX_MAX 64
#define MQTT_DUMP_RX_MAX KB(64)

internal u8 mqtt_dump_rx_buffer[MQTT_DUMP_RX_MAX];

internal b32 mqtt_dump_handle(const MqttFixedHeader header, const str body)
{
    MqttReader r = mqtt_reader_create(body.data, body.length);

    switch(header.type)
    {
        case MQTT_SUBSCRIBE_ACK:
        {
            const u16 packet_id = mqtt_read_u16_be(&r);
            const u8 return_code = mqtt_read_u8(&r);
            if (r.error || packet_id != 1 || return_code != MQTT_SUBSCRIBE_ACK_QOS_0)
            {
                fprintf(stderr, "SUBACK: refused (packet id %d, return code 0x%02x)\n", packet_id, return_code);
                return 0;
            }
            printf("subscribed to %s\n", MQTT_DUMP_TOPIC_FILTER);
            return 1;
        }
        case MQTT_PUBLISH:
        {
            const MqttPublish m = mqtt_read_publish(&r, header.flags);
            if (r.error)
            {
                fprintf(stderr, "malformed PUBLISH of %llu bytes\n", (unsigned long long)body.length);
                return 0;
            }
            printf("%.*s = %.*s%s\n", STR_FMT(m.topic), STR_FMT(m.payload), m.retain ? " (retained)" : "");
            return 1;
        }
        case MQTT_PINGRESP:
            return 1;
        default:
            fprintf(stderr, "unexpected packet type %d, ignored\n", header.type);
            return 1;
    }
}

internal b32 mqtt_dump_receive(MqttClient *c)
{
    const ssize_t received = recv(c->sock, mqtt_dump_rx_buffer, sizeof(mqtt_dump_rx_buffer), 0);
    if (received < 0 && errno == EAGAIN)
    {
        u8 packet[MQTT_DUMP_TX_MAX];
        MqttWriter w = mqtt_writer_create(packet, sizeof(packet));
        mqtt_write_pingreq(&w);
        return mqtt_client_send(c, &w);
    }
    if (received <= 0)
    {
        fprintf(stderr, "the broker closed the connection\n");
        return 0;
    }

    MqttReader r = mqtt_reader_create(mqtt_dump_rx_buffer, (u64)received);
    while (r.position < r.length){
        const MqttFixedHeader header = mqtt_read_fixed_header(&r);
        const str body = mqtt_read_bytes(&r, header.remaining_length);
        if (r.error)
        {
            fprintf(stderr, "a packet is cut off at the end of %zd received bytes\n", received);
            return 0;
        }
        if (!mqtt_dump_handle(header, body))
        {
            return 0;
        }
    }
    return 1;
}

int main(void) 
{
    setvbuf(stdout, 0, _IOLBF, 0);

    MqttClient client = mqtt_client_connect(MQTT_DUMP_BROKER_IP, MQTT_DUMP_BROKER_PORT, STR_LITERAL(MQTT_DUMP_CLIENT_ID), MQTT_DUMP_KEEP_ALIVE, STR_EMPTY, STR_EMPTY);
    if (client.sock < 0)
    {
        return 1;
    }

    const struct timeval keep_alive = { MQTT_DUMP_KEEP_ALIVE, 0 };
    setsockopt(client.sock, SOL_SOCKET, SO_RCVTIMEO, &keep_alive, sizeof(keep_alive));

    u8 packet[MQTT_DUMP_TX_MAX];
    MqttWriter w = mqtt_writer_create(packet, sizeof(packet));
    mqtt_write_subscribe(&w, 1, STR_LITERAL(MQTT_DUMP_TOPIC_FILTER));
    if (!mqtt_client_send(&client, &w))
    {
        close(client.sock);
        return 1;
    }

    while(mqtt_dump_receive(&client)){}
    close(client.sock);
    return 1;
}