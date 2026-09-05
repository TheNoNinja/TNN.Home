#include "base/base.h"
#include "histogram/histogram.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#define ECHO_PROBE_BROKER_IP "127.0.0.1"
#define ECHO_PROBE_BROKER_PORT 1883
#define ECHO_PROBE_CLIENT_ID "tnn-echo-probe"
#define ECHO_PROBE_TOPIC "tnn/echo-probe"
#define ECHO_PROBE_KEEP_ALIVE 10
#define ECHO_PROBE_COUNT 60000
#define ECHO_PROBE_RATE 1000
#define ECHO_PROBE_RATE_MAX 100000
#define ECHO_PROBE_TIMEOUT_MS 1000
#define ECHO_PROBE_TX_MAX 64
#define ECHO_PROBE_PAYLOAD_MAX 16
#define ECHO_PROBE_RX_MAX KB(64)
#define ECHO_PROBE_RX_PACKET_MAX KB(64)
#define ECHO_PROBE_LINE_MAX 256

#ifdef NDEBUG
#define ECHO_PROBE_BUILD "release"
#else 
#define ECHO_PROBE_BUILD "debug"
#endif

internal u8 echo_probe_rx_buffer[ECHO_PROBE_RX_MAX];
internal u8 echo_probe_tx_buffer[ECHO_PROBE_TX_MAX];

typedef struct EchoProbe EchoProbe;
struct EchoProbe 
{
    MqttClient client;
    Loop loop;
    int tick;
    u32 timeout;
    u32 count;
    u32 rate;
    u64 interval_ns;
    u64 start_ns;
    u32 sent;
    u32 received;
    u32 lost;
    Histogram round_trip;
    Histogram latency;
};

internal b32 echo_probe_send(EchoProbe *p)
{
    u8 payload[ECHO_PROBE_PAYLOAD_MAX];
    MqttWriter body = mqtt_writer_create(payload, sizeof(payload));
    mqtt_write_u64_be(&body, time_now_raw_ns());
    mqtt_write_u64_be(&body, p->sent);

    const str message = { payload, body.length };
    p->sent += 1;
    if (p->rate == 0 || p->sent == p->count)
    {
        loop_timer_start(&p->loop, p->timeout, ECHO_PROBE_TIMEOUT_MS);
    }
    return mqtt_client_publish(&p->client, STR(ECHO_PROBE_TOPIC), message, 0);
}

internal b32 echo_probe_next(EchoProbe *p)
{
    return p->sent < p->count ? echo_probe_send(p) : 1;
}

internal void echo_probe_tick_start(EchoProbe *p)
{
    struct itimerspec spec = {0};
    spec.it_value.tv_sec = (time_t)(p->interval_ns / TIME_NS_PER_S);
    spec.it_value.tv_nsec = (long)(p->interval_ns % TIME_NS_PER_S);
    spec.it_interval = spec.it_value;
    p->start_ns = time_now_mono_ns() + p->interval_ns;
    timerfd_settime(p->tick, 0, &spec, 0);
}

internal b32 echo_probe_tick(EchoProbe *p)
{
    u64 ticks = 0;
    if (read(p->tick, &ticks, sizeof(ticks)) < 0)
    {
        LOG_ERROR("read the tick: %s", strerror(errno));
        return 0;
    }
    for(u64 i = 0; i < ticks && p->sent < p->count; i++)
    {
        if(!echo_probe_send(p))
        {
            return 0;
        }
    }
    return 1;
}

internal b32 echo_probe_give_up(EchoProbe *p)
{
    if (p->rate == 0)
    {
        p->lost += 1;
        LOG_WARN("no echo for message %u within %d ms", p->sent -1, ECHO_PROBE_TIMEOUT_MS);
        return echo_probe_next(p);
    }
    p->lost = p->sent - p->received;
    LOG_WARN("%u echos still out %d ms after the last send", p->lost, ECHO_PROBE_TIMEOUT_MS);
    return 1;
}

internal b32 echo_probe_done(const EchoProbe *p)
{
    return p->received + p->lost >= p->count;
}

internal b32 echo_probe_handle(EchoProbe *p, const MqttPacket *packet)
{
    switch(packet->type)
    {
        case MQTT_SUBSCRIBE_ACK:
        {
            if (p->rate == 0)
            {
                LOG_INFO("subscribed to %s: sending %u messages, one at a time", ECHO_PROBE_TOPIC, p->count);
                return echo_probe_send(p);
            }
            LOG_INFO("subscribed to %s: sending %u messages, %u per second", ECHO_PROBE_TOPIC, p->count, p->rate);
            echo_probe_tick_start(p);
            return 1;
        }
        case MQTT_PUBLISH:
        {
            const u64 now_ns = time_now_raw_ns();
            const u64 now_mono_ns = time_now_mono_ns();
            const str message = packet->publish.payload;
            MqttReader payload = mqtt_reader_create(message.data, message.length);
            const u64 sent_ns = mqtt_read_u64_be(&payload);
            const u64 sequence = mqtt_read_u64_be(&payload);
            if (payload.error)
            {
                LOG_ERROR("malformed echo of %llu bytes", (unsigned long long) message.length);
                return 0;
            }


            const b32 waited_for = p->rate == 0 ? sequence + 1 == p->sent : sequence < p->sent;
            if (!waited_for)
            {
                LOG_WARN("echo of message %llu is not the one we wait for, ignored", (unsigned long long)sequence);
                return 1;
            }
            p->received += 1;

            histogram_record(&p->round_trip, now_ns - sent_ns);
            if (p->rate == 0)
            {
                return echo_probe_next(p);
            }
            histogram_record(&p->latency, now_mono_ns - (p->start_ns + sequence * p->interval_ns));
            return 1;
        }
        default:
        {
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

    MqttPacket packet = {0};
    while (mqtt_client_next(&p->client, &packet))
    {
        if(!echo_probe_handle(p, &packet))
        {
            return 0;
        }
    }

    return !p->client.error;
}

internal void echo_probe_print(const EchoProbe *p, const u64 port)
{
    char date[32];
    const time_t now = time(0);
    struct tm local;
    localtime_r(&now, &local);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M", &local);

    printf("echo probe: port %llu, count %u, rate %u, %s build, %s\n", (unsigned long long)port, p->count, p->rate, ECHO_PROBE_BUILD, date);
    if (p->rate > 0)
    {
        histogram_print(&p->latency, "latency");
    }
    histogram_print(&p->round_trip, "round trip");
    printf("  lost  %10u\n", p->lost);

}

internal b32 echo_probe_compare_line(const char *before, const char *after)
{
    char name_before[16];
    char name_after[16];
    char unit[8] = "";
    f64 value_before = 0.0;
    f64 value_after = 0.0;
    if (sscanf(before, "%15s %lf %7s", name_before, &value_before, unit) < 2 || sscanf(after, "%15s %lf", name_after, &value_after) != 2 || strcmp(name_before, name_after) != 0)
    {
        return 0;
    }
    const int decimals = unit[0] != '\0' ? 1 : 0;
    printf("  %-6s %10.*f %-2s -> %10.*f", name_before, decimals, value_before, unit, decimals, value_after);
    if (value_before > 0.0)
    {
        printf(" %-2s %+8.1f %%", unit, (value_after - value_before) / value_before * 100.0);
    }
    printf("\n");
    return 1;
}

internal b32 echo_probe_compare(const char *before_path, const char *after_path)
{
    FILE *before = fopen(before_path, "r");
    if (!before)
    {
        LOG_ERROR("cannot open %s: %s", before_path, strerror(errno));
        return 0;
    }

    FILE *after = fopen(after_path, "r");
    if (!after)
    {
        LOG_ERROR("cannot open %s: %s", after_path, strerror(errno));
        fclose(before);
        return 0;
    }

    char a[ECHO_PROBE_LINE_MAX];
    char b[ECHO_PROBE_LINE_MAX];
    b32 ok = 1;
    for (u32 line = 1; ok; line++)
    {
        const b32 more_a = fgets(a, sizeof(a), before) != 0;
        const b32 more_b = fgets(b, sizeof(b), after) != 0;
        if (!more_a || !more_b)
        {
            if(more_a != more_b)
            {
                LOG_ERROR("%s ends at line %u, the other file goes on", more_a ? after_path : before_path, line);
                ok = 0;
            }
            break;
        }

        a[strcspn(a, "\n")] = '\0';
        b[strcspn(b, "\n")] = '\0';
        if (a[0] == ' ')
        {
            ok = echo_probe_compare_line(a, b);
        }
        else 
        {
            const u64 name = strcspn(a, ":");
            ok = name == strcspn(b, ":") && strncmp(a, b, name) == 0;
            if (ok && strcmp(a, b) == 0)
            {
                printf("%s\n", a);
            }
            else if (ok)
            {
                printf("before: %s\nafter:  %s\n", a, b);
            }
        }
        if (!ok)
        {
            LOG_ERROR("line %u does not match: '%s' and '%s'", line, a, b);
        }
    }
    fclose(before);
    fclose(after);
    return ok;
}

internal b32 echo_probe_number(const char *text, const u64 min, const u64 max, u64 *value)
{
    return str_to_u64(str_from_cstr(text), value) && *value >= min && *value <= max;
}

int main(const int argc, char **argv)
{
    if (argc == 4 && strcmp(argv[1], "compare") == 0)
    {
        return echo_probe_compare(argv[2], argv[3]) ? 0 : 1;
    }

    u64 port = ECHO_PROBE_BROKER_PORT;
    u64 count = ECHO_PROBE_COUNT;
    u64 rate = ECHO_PROBE_RATE;
    if (argc > 4 || (argc > 1 && !echo_probe_number(argv[1], 1, 65535, &port)) || (argc > 2 && !echo_probe_number(argv[2], 1, 1000000, &count)) || (argc > 3 && !echo_probe_number(argv[3], 0, ECHO_PROBE_RATE_MAX, &rate)))
    {
        LOG_ERROR("usage: %s [port(1 - 65535, default %d)] [count(1 - 1000000, default %d)] [rate(0 - %d, default %d)]", argv[0], ECHO_PROBE_BROKER_PORT, ECHO_PROBE_COUNT, ECHO_PROBE_RATE_MAX, ECHO_PROBE_RATE);
        LOG_ERROR("usage: %s compare <before> <after>", argv[0]);
        return 1;
    }

    EchoProbe p = {0};
    p.count = (u32)count;
    p.rate = rate;
    p.interval_ns = rate > 0 ? TIME_NS_PER_S / rate : 0;
    p.round_trip = histogram_create();
    p.latency = histogram_create();
    p.client = mqtt_client_create(echo_probe_rx_buffer, sizeof(echo_probe_rx_buffer), ECHO_PROBE_RX_PACKET_MAX, echo_probe_tx_buffer, sizeof(echo_probe_tx_buffer));
    if (!mqtt_client_connect(&p.client, ECHO_PROBE_BROKER_IP, (u16)port, STR(ECHO_PROBE_CLIENT_ID), ECHO_PROBE_KEEP_ALIVE, STR_EMPTY, STR_EMPTY))
    {
        return 1;
    }

    b32 ok = mqtt_client_subscribe(&p.client, STR(ECHO_PROBE_TOPIC)) != 0;

    p.loop = loop_create();
    loop_catch_signals();
    loop_watch(&p.loop, p.client.sock);
    p.timeout = loop_timer_add(&p.loop);
    p.tick = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (p.tick < 0)
    {
        LOG_ERROR("timerfd_create: %s", strerror(errno));
        ok = 0;
    }

    loop_watch(&p.loop, p.tick);

    while(ok && !echo_probe_done(&p) && loop_wait(&p.loop))
    {
        if (loop_timer_fired(&p.loop, p.timeout))
        {
            ok = echo_probe_give_up(&p);
        }

        if (ok && loop_readable(&p.loop, p.tick))
        {
            ok = echo_probe_tick(&p);
        }

        if (ok && !echo_probe_done(&p) && loop_readable(&p.loop, p.client.sock))
        {
            ok = echo_probe_receive(&p);
        }
    }

    if (ok && p.loop.stop_signal != 0)
    {
        LOG_INFO("signal %d (%s): stopping after %u of %u messages", p.loop.stop_signal, strsignal(p.loop.stop_signal), p.sent, p.count);
    }
    ok = ok && (p.loop.stop_signal != 0 || echo_probe_done(&p));
    if (ok)
    {
        echo_probe_print(&p, port);
        ok = mqtt_client_disconnect(&p.client);
    }
    close(p.tick);
    mqtt_client_close(&p.client);
    return ok ? 0 : 1;
}