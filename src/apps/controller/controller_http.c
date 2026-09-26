#include "apps/controller/controller.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define CONTROLLER_HTTP_BACKLOG 16

internal const char controller_http_hello[] = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain; charset=utf-8\r\n"
    "Content-Lenght: 20\r\n"
    "Connection: close\r\n"
    "\r\n"
    "TNN.Home controller\r\n";

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
    loop_watch(&c->loop, sock);
}

internal void controller_http_receive(Controller *c, ControllerHttpConnection *h)
{
    u8 bytes[1024];
    const ssize_t received = recv(h->sock, bytes, sizeof(bytes), 0);
    if (received < 0 && (errno == EAGAIN || errno == EINTR))
    {
        return;
    }
    if(received > 0)
    {
        const ssize_t sent = send(h->sock, controller_http_hello, sizeof(controller_http_hello) - 1, MSG_NOSIGNAL);
        LOG_INFO("a request of %lld bytes, answered with %lld bytes", (long long)received, (long long)sent);
    }
    controller_http_close(c, h);
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