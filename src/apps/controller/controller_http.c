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
        LOG_ERROR("socket: %s", strerror(errno));
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
        LOG_ERROR("bind to %s:%d: %s", ip, port, strerror(errno));
        goto error_handling;
    }

    if (listen(c->http_sock, CONTROLLER_HTTP_BACKLOG) < 0)
    {
        LOG_ERROR("http listen: %s", strerror(errno));
        goto error_handling;
    }

    fcntl(c->http_sock, F_SETFL, fcntl(c->http_sock, F_GETFL) | O_NONBLOCK);
    loop_watch(&c->client.loop, c->http_sock);
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
    loop_unwatch(&c->client.loop, h->sock);
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
    Writer w = writer_create(controller_http_response, sizeof(controller_http_response));
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
    write_bytes(&w, body);
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
    Writer body = writer_create(buffer, sizeof(buffer));
    write_u64_str(&body, status);
    write_bytes(&body, STR(" "));
    write_bytes(&body, http_status_reason(status));
    write_bytes(&body, STR("\n"));
    controller_http_send(sock, status, allow, STR("text/plain; charset=utf-8"), writer_to_str(&body));
}

internal void controller_http_accept(Controller *c)
{
    const int sock = accept(c->http_sock, 0, 0);
    if (sock < 0)
    {
        if (errno != EAGAIN && errno != EINTR)
        {
            LOG_WARN("accept: %s", strerror(errno));
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
    h->opened_ms = c->client.loop.now_ms;
    h->length = 0;
    loop_watch(&c->client.loop, sock);
}

internal b32 controller_http_host_valid(const Controller *c, const str host)
{
    str name = STR_EMPTY;
    str digits = STR_EMPTY;
    u64 port = 0;
    if (!str_cut(host, ':', &name, &digits) || !str_to_u64(digits, &port) || port != c->http_port)
    {
        return 0;
    }
    return str_equal(name, c->http_ip) || str_equal(name, STR("localhost"));
}

internal b32 controller_http_origin_valid(const HttpRequest *request)
{
    const str scheme = STR("http://");
    const str origin = request->origin;
    if (origin.length == 0)
    {
        return 1;
    }
    return str_starts_with(origin, scheme) && str_equal(str_skip(origin, scheme.length), request->host);
}

internal u32 controller_http_set(Controller *c, const HttpRequest *request, b32 *is_connected)
{
    if (!controller_http_origin_valid(request))
    {
        return 403;
    }
    switch(controller_set(c, request->body))
    {
        case CONTROLLER_SET_SENT: return 204;
        case CONTROLLER_SET_REFUSED: return 422;
        case CONTROLLER_SET_MALFORMED: return 400;
        case CONTROLLER_SET_LOST: *is_connected = 0; return 500;
    }
    return 500;
}

internal b32 controller_http_request(Controller *c, ControllerHttpConnection *h, const HttpRequest *request)
{
    const b32 is_page = str_equal(request->target, STR("/"));
    const b32 is_events = str_equal(request->target, STR("/events"));
    const b32 is_set = str_equal(request->target, STR("/set"));
    const str method = is_set ? STR("POST") : STR("GET");

    u32 status = 200;
    b32 is_connected = 1;
    Writer page = writer_create(controller_http_page, sizeof(controller_http_page));
    if (!controller_http_host_valid(c, request->host))
    {
        status = 403;
    } 
    else if (!is_page && !is_events && !is_set)
    {
        status = 404;
    }
    else if (!str_equal(request->method, method))
    {
        status = 405;
    }
    else if (is_set)
    {
        status = controller_http_set(c, request, &is_connected);
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
        return 1;
    }
    else if(status == 200)
    {
        controller_http_send(h->sock, status, STR_EMPTY, STR("text/html; charset=utf-8"), writer_to_str(&page));
    }
    else if (status == 204)
    {
        controller_http_send(h->sock, status, STR_EMPTY, STR_EMPTY, STR_EMPTY);
    }
    else 
    {
        controller_http_send_status(h->sock, status, method);
    }
    controller_http_close(c, h);
    return is_connected;
}

internal b32 controller_http_receive(Controller *c, ControllerHttpConnection *h)
{
    if (h->is_stream)
    {
        const ssize_t received = recv(h->sock, h->request, sizeof(h->request), 0);
        if (received == 0 || (received < 0 && errno != EAGAIN && errno != EINTR))
        {
            controller_http_close(c, h);
        }
        return 1;
    }

    const ssize_t received = recv(h->sock, h->request + h->length, sizeof(h->request) - h->length, 0);
    if (received < 0 && (errno == EAGAIN || errno == EINTR))
    {
        return 1;
    }
    if (received <= 0)
    {
        controller_http_close(c, h);
        return 1;
    }

    h->length += (u64)received;
    
    HttpRequest request = {0};
    switch (http_request_read((str){ h->request, h->length }, CONTROLLER_LINE_MAX, &request))
    {
        case HTTP_READ_OK:
            return controller_http_request(c, h, &request);
        case HTTP_READ_TOO_LARGE:
            LOG_WARN("a request body of more than %d bytes -> 413", CONTROLLER_LINE_MAX);
            controller_http_send_status(h->sock, 413, STR_EMPTY);
            controller_http_close(c, h);
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
                controller_http_close(c, h);
            }
            break;
    }
    return 1;
}

b32 controller_http_handle(Controller *c)
{
    for (u32 i = 0; i < CONTROLLER_HTTP_CONNECTION_MAX; i++)
    {
        ControllerHttpConnection *h = &c->http[i];
        if(h->is_open && loop_readable(&c->client.loop, h->sock) && !controller_http_receive(c, h))
        {
            return 0;
        }
    }
    if (loop_readable(&c->client.loop, c->http_sock))
    {
        controller_http_accept(c);
    }
    return 1;
}

