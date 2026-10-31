#include "apps/controller/controller.h"

#define CONTROLLER_EVENTS_SECTION_MAX KB(32)

internal u8 controller_events_section[CONTROLLER_EVENTS_SECTION_MAX];
internal u8 controller_events_buffer[CONTROLLER_HTTP_PAGE_MAX];

internal void controller_events_send(Controller *c, ControllerHttpConnection *h, const str bytes)
{
    const i64 sent = platform_send(h->sock, bytes);
    if (sent < 0)
    {
        LOG_WARN("an event stream: %s", platform_error());
        controller_http_close(c, h);
    }
    else if ((u64)sent != bytes.length)
    {
        LOG_WARN("an event stream took only %lld of %llu bytes", (long long)sent, (unsigned long long)bytes.length);
        controller_http_close(c, h);
    }
}

internal void controller_events_write_device(Writer *w, const ControllerDevice *d, const b32 is_removed)
{
    Writer section = writer_create(controller_events_section, sizeof(controller_events_section));
    if (is_removed)
    {
        controller_page_removed(&section, d);
    }
    else 
    {
        controller_page_device(&section, d);
    }
    ASSERT(!section.has_overflown);
    http_write_event(w, writer_to_str(&section));
}

void controller_events_open(Controller *c, ControllerHttpConnection *h)
{
    Writer w = writer_create(controller_events_buffer, sizeof(controller_events_buffer));
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
            controller_events_write_device(&w, d, 0);
        }
    }
    if (w.has_overflown)
    {
        LOG_ERROR("the first events do not fit in %llu bytes", (unsigned long long)w.capacity);
        controller_http_close(c, h);
        return;
    }
    h->is_stream = 1;
    controller_events_send(c, h, writer_to_str(&w));
}

internal void controller_events_broadcast(Controller *c, const ControllerDevice *d, const b32 is_removed)
{
    b32 has_stream = 0;
    for(u32 i = 0; i < CONTROLLER_HTTP_CONNECTION_MAX; i++)
    {
        if(c->http[i].is_stream)
        {
            has_stream = 1;
            break;
        }
    }
    if(!has_stream)
    {
        return;
    }
    Writer w = writer_create(controller_events_buffer, sizeof(controller_events_buffer));
    controller_events_write_device(&w, d, is_removed);
    for (u32 i = 0; i < CONTROLLER_HTTP_CONNECTION_MAX; i++)
    {
        ControllerHttpConnection *h = &c->http[i];
        if (h->is_stream)
        {
            controller_events_send(c, h, writer_to_str(&w));
        }
    }
}

void controller_events_device_changed(Controller *c, const ControllerDevice *d)
{
    controller_events_broadcast(c, d, 0);
}

void controller_events_device_removed(Controller *c, const ControllerDevice *d)
{
    controller_events_broadcast(c, d, 1);
}

void controller_events_heartbeat(Controller *c)
{
    for (u32 i = 0; i < CONTROLLER_HTTP_CONNECTION_MAX; i++)
    {
        ControllerHttpConnection *h = &c->http[i];
        if (h->is_stream)
        {
            controller_events_send(c, h, STR(": heartbeat\n\n"));
        }
    }
}