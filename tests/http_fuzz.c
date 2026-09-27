#include "base/base.h"
#include "http/http.h"

#include <stddef.h>
#include <string.h>

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
    const HttpRead result = http_request_read((str){ (u8 *)data, size }, &request);
    ASSERT(result == HTTP_READ_INCOMPLETE || result == HTTP_READ_OK || result == HTTP_READ_BAD);
    if (result != HTTP_READ_OK)
    {
        return 0;
    }

    const u64 head = request.head_length;
    ASSERT(head >= 4 && head <= size && memcmp(data + head - 4, "\r\n\r\n", 4) == 0);
    ASSERT(http_fuzz_word(request.method, data, head));
    ASSERT(http_fuzz_word(request.target, data, head) && request.target.data[0] == '/');
    ASSERT(http_fuzz_word(request.host, data, head));

    HttpRequest again = {0};
    ASSERT(http_request_read((str){ (u8 *)data, size }, &again) == HTTP_READ_OK);
    ASSERT(again.head_length == head && again.content_length == request.content_length);
    ASSERT(str_equal(again.method, request.method) && str_equal(again.target, again.target) && str_equal(again.host, request.host));
    ASSERT(http_request_read((str){ (u8 *)data, head - 1 }, &again) == HTTP_READ_INCOMPLETE);
    return 0;
}