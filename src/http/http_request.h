#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "base/base.h"

enum HttpRead
{
    HTTP_READ_INCOMPLETE,
    HTTP_READ_OK,
    HTTP_READ_BAD
};
typedef enum HttpRead HttpRead;

typedef struct HttpRequest HttpRequest;
struct HttpRequest
{
    str method;
    str target;
    u64 head_length;
};

HttpRead http_request_read(str bytes, HttpRequest *request);

#endif