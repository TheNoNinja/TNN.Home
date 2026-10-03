#include "apps/controller/controller.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define CONTROLLER_HTTP_BACKLOG 16
#define CONTROLLER_HTTP_HEAD_MAX 256

internal u8 controller_http_response[CONTROLLER_HTTP_HEAD_MAX + CONTROLLER_HTTP_BODY_MAX];
internal u8 controller_http_page[CONTROLLER_HTTP_BODY_MAX];

b32 controller_http_listen(Controller *c, const char* ip, const u16 port)
{
    c->http_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (c->http_sock < 0)
    {
        LOG_ERROR("http socket: %s", strerror(errno));
        return 0;
    }

    const int one = 1;
    setsockopt(c->http_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);
    if (bind(c->http_sock, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        LOG_ERROR("http bind to %s:%d: %s", ip, port, strerror(errno));
        goto error_handling;
    }

    if (listen(c->http_sock, CONTROLLER_HTTP_BACKLOG) < 0)
    {
        LOG_ERROR("http listen: %s", strerror(errno));
        goto error_handling;
    }

    fcntl(c->http_sock, F_SETFL, fcntl(c->http_sock, F_GETFL) | O_NONBLOCK);
    loop_watch(&c->loop, c->http_sock);
    c->http_ip = str_from_cstr(ip);
    c->http_port = port;
    LOG_INFO("listening on http://%s:%d/", ip, port);
    return 1;

error_handling:
    close(c->http_sock);
    c->http_sock = -1;
    return 0;
}

void controller_http_close(Controller *c, ControllerHttpConnection *h)
{
    if (h->is_stream)
    {
        LOG_INFO("an event stream closed");
    }
    loop_unwatch(&c->loop, h->sock);
    close(h->sock);
    h->is_open = 0;
    h->is_stream = 0;
}

internal ControllerHttpConnection *controller_http_slot(Controller *c)
{
    ControllerHttpConnection *oldest = 0;
    for (u32 i = 0; i < CONTROLLER_HTTP_CONNECTION_MAX; i++)
    {
        ControllerHttpConnection *h = &c->http[i];
        if (!h->is_open)
        {
            return h;
        }
        if (!h->is_stream && (!oldest || h->opened_ms < oldest->opened_ms))
        {
            oldest = h;
        }
    }
    if (!oldest)
    {
        return 0;
    }

    LOG_WARN("%d conenctions are open: the oldest one is closing", CONTROLLER_HTTP_CONNECTION_MAX);
    controller_http_close(c, oldest);
    return oldest;
}

internal void controller_http_send(const int sock, const u32 status, const str allow, const str content_type, const str body)
{
    HttpWriter w = http_writer_create(controller_http_response, sizeof(controller_http_response));
    http_write_status(&w, status);
    if (status != 204)
    {
        http_write_header(&w, STR("Content-Type"), content_type);
        http_write_header_u64(&w, STR("Content-Length"), body.length);
    }
    http_write_header(&w, STR("Cache-Control"), STR("no-store"));
    http_write_header(&w, STR("Connection"), STR("close"));
    if (status == 405)
    {
        http_write_header(&w, STR("Allow"), allow);
    }
    http_write_head_end(&w);
    http_write_str(&w, body);
    ASSERT(!w.has_overflown);

    const ssize_t sent = send(sock, w.data, w.length, MSG_NOSIGNAL);
    if (sent < 0)
    {
        LOG_WARN("send: %s", strerror(errno));
    } 
    else if ((u64)sent != w.length)
    {
        LOG_WARN("send: only %lld of %llu bytes went out (the buffer is full)", (long long)sent, (unsigned long long)w.length);
    }
}

internal void controller_http_send_status(const int sock, const u32 status, const str allow)
{
    u8 buffer[64];
    HttpWriter body = http_writer_create(buffer, sizeof(buffer));
    http_write_u64(&body, status);
    http_write_str(&body, STR(" "));
    http_write_str(&body, http_status_reason(status));
    http_write_str(&body, STR("\n"));
    controller_http_send(sock, status, allow, STR("test/plain; charset=utf-8"), http_writer_str(&body));
}

internal void controller_http_accept(Controller *c)
{
    const int sock = accept(c->http_sock, 0, 0);
    if (sock < 0)
    {
        if (errno != EAGAIN && errno != EINTR)
        {
            LOG_WARN("http accept: %s", strerror(errno));
        }
        return;
    }
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);

    ControllerHttpConnection *h = controller_http_slot(c);
    if (!h)
    {
        LOG_WARN("%d event streams are open: a new connection gets 503", CONTROLLER_HTTP_CONNECTION_MAX);
        controller_http_send_status(sock, 503, STR_EMPTY);
        close(sock);
        return;
    }
    h->is_open = 1;
    h->sock = sock;
    h->opened_ms = c->loop.now_ms;
    h->length = 0;
    loop_watch(&c->loop, sock);
}

internal void controller_http_request(Controller *c, ControllerHttpConnection *h, const HttpRequest *request)
{
    const b32 is_page = str_equal(request->target, STR("/"));
    const b32 is_events = str_equal(request->target, STR("/events"));

    u32 status = 200;
    HttpWriter page = http_writer_create(controller_http_page, sizeof(controller_http_page));
    if (!is_page && !is_events)
    {
        status = 404;
    }
    else if (!str_equal(request->method, STR("GET")))
    {
        status = 405;
    }
    else if (is_page)
    {
        controller_page_write(c, &page);
        if (page.has_overflown)
        {
            LOG_ERROR("the page does not fit in %llu bytes", (unsigned long long)page.capacity);
            status = 500;
        }    
    }

    LOG_INFO("%.*s %.*s -> %u", STR_FMT(request->method), STR_FMT(request->target), status);
    if (status == 200 && is_events)
    {
        controller_events_open(c, h);
        return;
    }
    else if(status == 200)
    {
        controller_http_send(h->sock, status, STR_EMPTY, STR("text/html; charset=utf-8"), http_writer_str(&page));
    }
    else 
    {
        controller_http_send_status(h->sock, status, STR("GET"));
    }
    controller_http_close(c, h);
}

internal void controller_http_receive(Controller *c, ControllerHttpConnection *h)
{
    if (h->is_stream)
    {
        const ssize_t received = recv(h->sock, h->request, sizeof(h->request), 0);
        if (received == 0 || (received < 0 && errno == EAGAIN && errno != EINTR))
        {
            controller_http_close(c, h);
        }
        return;
    }

    const ssize_t received = recv(h->sock, h->request + h->length, sizeof(h->request) - h->length, 0);
    if (received < 0 && (errno == EAGAIN || errno == EINTR))
    {
        return;
    }
    if (received <= 0)
    {
        controller_http_close(c, h);
        return;
    }

    h->length += (u64)received;
    
    HttpRequest request = {0};
    switch (http_request_read((str){ h->request, h->length }, &request))
    {
        case HTTP_READ_OK:
            controller_http_request(c, h, &request);
            break;
        case HTTP_READ_BAD:
            LOG_WARN("a request that breaks the rules of HTTP/1.1 -> 400");
            controller_http_send_status(h->sock, 400, STR_EMPTY);
            controller_http_close(c, h);
            break;
        case HTTP_READ_INCOMPLETE:
            if (h->length == sizeof(h->request))
            {
                LOG_WARN("a request head of more than %llu bytes -> 431", (unsigned long long)sizeof(h->request));
                controller_http_send_status(h->sock, 431, STR_EMPTY);
            }
            break;
    }
}

void controller_http_handle(Controller *c)
{
    for (u32 i = 0; i < CONTROLLER_HTTP_CONNECTION_MAX; i++)
    {
        ControllerHttpConnection *h = &c->http[i];
        if(h->is_open && loop_readable(&c->loop, h->sock))
        {
            controller_http_receive(c, h);
        }
    }
    if (loop_readable(&c->loop, c->http_sock))
    {
        controller_http_accept(c);
    }
}