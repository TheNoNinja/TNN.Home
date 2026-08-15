#include "base/base_log.h"
#include "base/base_time.h"
#include "base/base_types.h"
#include "histogram/histogram.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ECHO_PROBE_BROKER_IP "127.0.0.1"
#define ECHO_PROBE_BROKER_PORT 1883
#define ECHO_PROBE_CLIENT_ID "tnn-echo-probe"
#define ECHO_PROBE_TOPIC "tnn/echo-probe"
#define ECHO_PROBE_KEEP_ALIVE 10
#define ECHO_PROBE_COUNT 20000
#define ECHO_PROBE_TIMEOUT_MS 1000
#define ECHO_PROBE_TX_MAX 64
#define ECHO_PROBE_PAYLOAD_MAX 16
#define ECHO_PROBE_RX_MAX KB(64)
#define ECHO_PROBE_RX_PACKET_MAX KB(64)

internal u8 echo_probe_rx_buffer[ECHO_PROBE_RX_MAX];

typedef struct EchoProbe EchoProbe;
struct EchoProbe 
{
    MqttClient client;
    Loop loop;
    u32 timeout;
    u32 count;
    u32 sent;
    u32 lost;
    Histogram round_trip;
};

internal b32 echo_probe_send(EchoProbe *p)
{
    u8 payload[ECHO_PROBE_PAYLOAD_MAX];
    MqttWriter body = mqtt_writer_create(payload, sizeof(payload));
    mqtt_write_u64_be(&body, time_now_raw_ns());
    mqtt_write_u64_be(&body, p->sent);

    u8 packet[ECHO_PROBE_TX_MAX];
    MqttWriter w= mqtt_writer_create(packet, sizeof(packet));
    const str message = { payload, body.length };
    mqtt_write_publish(&w, STR_LITERAL(ECHO_PROBE_TOPIC), message, 0);
    p->sent += 1;
    loop_timer_start(&p->loop, p->timeout, ECHO_PROBE_TIMEOUT_MS);
    return mqtt_client_send(&p->client, &w);
}

internal b32 echo_probe_next(EchoProbe *p)
{
    return p->sent < p->count ? echo_probe_send(p) : 1;
}

internal b32 echo_probe_handle(EchoProbe *p, const MqttFixedHeader header, const str body)
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
                LOG_ERROR("SUBACK: refused (packet id %d, return code 0x%02x)", packet_id, return_code);
                return 0;
            }
            LOG_INFO("subscribed to %s: sending %d messages, one at a time,", ECHO_PROBE_TOPIC, p->count);
            return echo_probe_send(p);
        }
        case MQTT_PUBLISH:
        {
            const u64 now_ns = time_now_raw_ns();
            const MqttPublish m = mqtt_read_publish(&r, header.flags);
            MqttReader payload = mqtt_reader_create(m.payload.data, m.payload.length);
            const u64 sent_ns = mqtt_read_u64_be(&payload);
            const u64 sequence = mqtt_read_u64_be(&payload);
            if (r.error || payload.error)
            {
                LOG_ERROR("malformed PUBLISH of %llu bytes", (unsigned long long) body.length);
                return 0;
            }

            if (sequence + 1 != p->sent)
            {
                LOG_WARN("echo of message %llu is not the one in flight, ignored", (unsigned long long)sequence);
                return 1;
            }

            histogram_record(&p->round_trip, now_ns - sent_ns);
            return echo_probe_next(p);
        }
        default:
        {
            LOG_WARN("unexpected packet type %d, ignored", header.type);
            return 1;
        }
    }
}

internal b32 echo_probe_receive(EchoProbe *p)
{
    if (!mqtt_client_receive(&p->client))
    {
        return 0;
    }

    MqttFixedHeader header = {0};
    str body = STR_EMPTY;
    while (mqtt_stream_next(&p->client.in, &header, &body))
    {
        if(!echo_probe_handle(p, header, body))
        {
            return 0;
        }
    }

    return !p->client.in.error;
}

internal b32 echo_probe_number(const char *text, const long min, const long max, long *value)
{
    char *end = 0;
    *value = strtol(text, &end, 10);
    return *end == '\0' && *value >= min && *value <= max;
}

int main(const int argc, char **argv)
{
    long port = ECHO_PROBE_BROKER_PORT;
    long count = ECHO_PROBE_COUNT;
    if (argc > 3 || (argc > 1 && !echo_probe_number(argv[1], 1, 65535, &port)) || (argc > 2 && !echo_probe_number(argv[2], 1, 1000000, &count)))
    {
        LOG_ERROR("usage: %s [port(1 - 65535, default %d)] [count(1 - 1000000, default %d)]", argv[0], ECHO_PROBE_BROKER_PORT, ECHO_PROBE_COUNT);
        return 1;
    }

    EchoProbe p = {0};
    p.count = (u32)count;
    p.round_trip = histogram_create();
    p.client = mqtt_client_create(echo_probe_rx_buffer, sizeof(echo_probe_rx_buffer), ECHO_PROBE_RX_PACKET_MAX);
    if (!mqtt_client_connect(&p.client, ECHO_PROBE_BROKER_IP, (u16)port, STR_LITERAL(ECHO_PROBE_CLIENT_ID), ECHO_PROBE_KEEP_ALIVE, STR_EMPTY, STR_EMPTY))
    {
        return 1;
    }

    u8 packet[ECHO_PROBE_TX_MAX];
    MqttWriter w = mqtt_writer_create(packet, sizeof(packet));
    mqtt_write_subscribe(&w, 1, STR_LITERAL(ECHO_PROBE_TOPIC));
    b32 ok = mqtt_client_send(&p.client, &w);

    p.loop = loop_create();
    loop_catch_signals();
    loop_watch(&p.loop, p.client.sock);
    p.timeout = loop_timer_add(&p.loop);

    while(ok && p.round_trip.count < p.count && loop_wait(&p.loop))
    {
        if (loop_timer_fired(&p.loop, p.timeout))
        {
            p.lost += 1;
            LOG_WARN("no echo for message %u within %d ms", p.sent - 1, ECHO_PROBE_TIMEOUT_MS);
            ok = echo_probe_next(&p);
        }

        if (ok && loop_readable(&p.loop, p.client.sock))
        {
            ok = echo_probe_receive(&p);
        }
    }

    if (ok && p.loop.stop_signal != 0)
    {
        LOG_INFO("signal %d (%s): stopping after %u of %u messages", p.loop.stop_signal, strsignal(p.loop.stop_signal), p.sent, p.count);
    }
    ok = ok && (p.loop.stop_signal != 0 || p.round_trip.count >= p.count);
    if (ok)
    {
        histogram_print(&p.round_trip, "round trip");
        printf("  lost  %10u\n", p.lost);
        w = mqtt_writer_create(packet, sizeof(packet));
        mqtt_write_disconnect(&w);
        ok = mqtt_client_send(&p.client, &w);
    }
    close(p.client.sock);
    return ok ? 0 : 1;
}