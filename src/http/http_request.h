#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "base/base.h"

enum HttpRead
{
    HTTP_READ_INCOMPLETE,
    HTTP_READ_OK,
    HTTP_READ_BAD,
    HTTP_READ_TOO_LARGE,
};
typedef enum HttpRead HttpRead;

typedef struct HttpRequest HttpRequest;
struct HttpRequest
{
    str method;
    str target;
    str host;
    str origin;
    str body;
    u64 head_length;
};

HttpRead http_request_read(str bytes, u64 body_max, HttpRequest *request);
b32 http_request_host_is(const HttpRequest *request, str ip, u16 port);
b32 http_request_same_origin(const HttpRequest *request);

#endif