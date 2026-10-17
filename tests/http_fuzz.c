#include "base/base.h"
#include "http/http.h"

#include <stddef.h>
#include <string.h>

#define HTTP_FUZZ_BODY_MAX 256

int LLVMFuzzerTestOneInput(const u8 *data, size_t size);

internal b32 http_fuzz_word(const str s, const u8 *data, const u64 length)
{
    for (u64 i = 0; i < s.length; i++)
    {
        if (s.data[i] < 0x21 || s.data[i] > 0x7E)
        {
            return 0;
        }
    }
    return s.length > 0 && s.data >= data && s.data + s.length <= data + length;
}

int LLVMFuzzerTestOneInput(const u8 *data, const size_t size)
{
    HttpRequest request = {0};
    const HttpRead result = http_request_read((str){ (u8 *)data, size }, HTTP_FUZZ_BODY_MAX, &request);
    ASSERT(result == HTTP_READ_INCOMPLETE || result == HTTP_READ_OK || result == HTTP_READ_BAD || result == HTTP_READ_TOO_LARGE);
    if (result != HTTP_READ_OK)
    {
        return 0;
    }

    const u64 head = request.head_length;
    const u64 whole = head + request.body.length;
    ASSERT(head >= 4 && head <= size && memcmp(data + head - 4, "\r\n\r\n", 4) == 0);
    ASSERT(request.body.length <= HTTP_FUZZ_BODY_MAX && whole <= size);
    ASSERT(request.body.length == 0 || request.body.data == data + head);
    ASSERT(http_fuzz_word(request.method, data, head));
    ASSERT(http_fuzz_word(request.target, data, head) && request.target.data[0] == '/');
    ASSERT(http_fuzz_word(request.host, data, head));
    ASSERT(request.origin.length == 0 || http_fuzz_word(request.origin, data, head));

    HttpRequest again = {0};
    ASSERT(http_request_read((str){ (u8 *)data, whole }, HTTP_FUZZ_BODY_MAX, &again) == HTTP_READ_OK);
    ASSERT(again.head_length == head && str_equal(again.body, request.body));
    ASSERT(str_equal(again.method, request.method) && str_equal(again.target, request.target) && str_equal(again.host, request.host) && str_equal(again.origin, request.origin));
    ASSERT(http_request_read((str){ (u8 *)data, whole - 1 }, HTTP_FUZZ_BODY_MAX, &again) == HTTP_READ_INCOMPLETE);
    return 0;
}