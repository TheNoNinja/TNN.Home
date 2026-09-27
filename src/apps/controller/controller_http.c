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
#define CONTROLLER_HTTP_BODY_MAX KB(1024)

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
    LOG_INFO("listening on http://%s:%d/", ip, port);
    return 1;

error_handling:
    close(c->http_sock);
    c->http_sock = -1;
    return 0;
}

internal void controller_http_close(Controller *c, ControllerHttpConnection *h)
{
    loop_unwatch(&c->loop, h->sock);
    close(h->sock);
    h->is_open = 0;
}

internal ControllerHttpConnection *controller_http_slot(Controller *c)
{
    ControllerHttpConnection *oldest = &c->http[0];
    for (u32 i = 0; i < CONTROLLER_HTTP_CONNECTION_MAX; i++)
    {
        ControllerHttpConnection *h = &c->http[i];
        if (!h->is_open)
        {
            return h;
        }
        if (h->opened_ms < oldest->opened_ms)
        {
            oldest = h;
        }
    }

    LOG_WARN("%d conenctions are open: the oldest one is closing", CONTROLLER_HTTP_CONNECTION_MAX);
    controller_http_close(c, oldest);
    return oldest;
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
    h->is_open = 1;
    h->sock = sock;
    h->opened_ms = c->loop.now_ms;
    h->length = 0;
    loop_watch(&c->loop, sock);
}

internal void controller_http_respond(Controller *c, ControllerHttpConnection *h, const u32 status, const str content_type, const str body)
{
    HttpWriter w = http_writer_create(controller_http_response, sizeof(controller_http_response));
    http_write_status(&w, status);
    http_write_header(&w, STR("Content-Type"), content_type);
    http_write_header_u64(&w, STR("Content-Length"), body.length);
    http_write_header(&w, STR("Cache-Control"), STR("no-store"));
    http_write_header(&w, STR("Connection"), STR("Close"));
    if (status == 405)
    {
        http_write_header(&w, STR("Allow"), STR("GET"));
    }
    http_write_head_end(&w);
    http_write_str(&w, body);
    ASSERT(!w.has_overflown);
    const ssize_t sent = send(h->sock, w.data, w.length, MSG_NOSIGNAL);
    if (sent < 0)
    {
        LOG_WARN("http send: %s", strerror(errno));
    }
    else if ((u64)sent != w.length)
    {
        LOG_WARN("http send: only %lld of %llu bytes went out (the send buffer is full)", (long long) sent, (unsigned long long)w.length);
    }
    controller_http_close(c, h);
}

internal void controller_http_respond_status(Controller *c, ControllerHttpConnection *h, const u32 status)
{
    u8 buffer[64];
    HttpWriter body = http_writer_create(buffer, sizeof(buffer));
    http_write_u64(&body, status);
    http_write_str(&body, STR(" "));
    http_write_str(&body, http_status_reason(status));
    http_write_str(&body, STR("\n"));
    controller_http_respond(c, h, status, STR("text/plain; charset=utf-8"), http_writer_str(&body));
}

internal void controller_http_request(Controller *c, ControllerHttpConnection *h, const HttpRequest *request)
{
    u32 status = 200;
    HttpWriter page = http_writer_create(controller_http_page, sizeof(controller_http_page));
    if (!str_equal(request->target, STR("/")))
    {
        status = 404;
    }
    else if (!str_equal(request->method, STR("GET")))
    {
        status = 405;
    }
    else 
    {
        controller_page_write(c, &page);
        if (page.has_overflown)
        {
            LOG_ERROR("the page does not fit in %llu bytes", (unsigned long long)page.capacity);
            status = 500;
        }    
    }

    LOG_INFO("%.*s %.*s -> %u", STR_FMT(request->method), STR_FMT(request->target), status);
    if (status == 200)
    {
        controller_http_respond(c, h, status, STR("text/html; charset=utf-8"), http_writer_str(&page));
    }
    else 
    {
        controller_http_respond_status(c, h, status);
    }
}

internal void controller_http_receive(Controller *c, ControllerHttpConnection *h)
{
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
            controller_http_respond_status(c, h, 400);
            break;
        case HTTP_READ_INCOMPLETE:
            if (h->length == sizeof(h->request))
            {
                LOG_WARN("a request head of more than %llu bytes -> 431", (unsigned long long)sizeof(h->request));
                controller_http_respond_status(c, h, 431);
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