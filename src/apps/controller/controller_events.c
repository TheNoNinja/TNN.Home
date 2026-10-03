#include "apps/controller/controller.h"

#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>

#define CONTROLLER_EVENTS_SECTION_MAX KB(32)

internal u8 controller_events_section[CONTROLLER_EVENTS_SECTION_MAX];
internal u8 controller_events_buffer[CONTROLLER_HTTP_BODY_MAX];

internal void controller_events_send(Controller *c, ControllerHttpConnection *h, const str bytes)
{
    const ssize_t sent = send(h->sock, bytes.data, bytes.length, MSG_NOSIGNAL);
    if (sent < 0)
    {
        LOG_WARN("an event stream: %s", strerror(errno));
        controller_http_close(c, h);
    }
    else if ((u64)sent != bytes.length)
    {
        LOG_WARN("an event stream took only %lld of %llu bytes", (long long)sent, (unsigned long long)bytes.length);
        controller_http_close(c, h);
    }
}

internal void controller_events_write_device(HttpWriter *w, const ControllerDevice *d)
{
    HttpWriter section = http_writer_create(controller_events_section, sizeof(controller_events_section));
    controller_page_device(&section, d);
    ASSERT(!section.has_overflown);
    http_write_event(w, http_writer_str(&section));
}

void controller_events_open(Controller *c, ControllerHttpConnection *h)
{
    const int one = 1;
    setsockopt(h->sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    HttpWriter w = http_writer_create(controller_events_buffer, sizeof(controller_events_buffer));
    http_write_status(&w, 200);
    http_write_header(&w, STR("Content-Type"), STR("text/event-stream"));
    http_write_header(&w, STR("Cache-Control"), STR("no-store"));
    http_write_header(&w, STR("Connection"), STR("close"));
    http_write_head_end(&w);

    for (u32 i = 0; i < CONTROLLER_DEVICE_MAX; i++)
    {
        const ControllerDevice *d = &c->devices[i];
        if (d->id.length > 0)
        {
            controller_events_write_device(&w, d);
        }
    }
    if (w.has_overflown)
    {
        LOG_ERROR("the first events do not fit in %llu bytes", (unsigned long long)w.capacity);
        controller_http_close(c, h);
        return;
    }
    h->is_stream = 1;
    controller_events_send(c, h, http_writer_str(&w));
}