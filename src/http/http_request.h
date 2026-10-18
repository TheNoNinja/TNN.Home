#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "base/base.h"

enum HttpParse
{
    HTTP_PARSE_INCOMPLETE,
    HTTP_PARSE_OK,
    HTTP_PARSE_BAD,
    HTTP_PARSE_TOO_LARGE,
};
typedef enum HttpParse HttpParse;

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

HttpParse http_request_parse(str bytes, u64 body_max, HttpRequest *request);
b32 http_request_host_is(const HttpRequest *request, str ip, u16 port);
b32 http_request_same_origin(const HttpRequest *request);

#endif