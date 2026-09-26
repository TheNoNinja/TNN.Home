#include "test.h"
#include "http/http.h"

void http_test_request_peices(void)
{
    const str curl = STR(
        "GET / HTTP/1.1\r\n"
        "Host: 127.0.0.1:8080\r\n"
        "Accept: */*\r\n"
        "\r\n"
    );
    HttpRequest request = {0};
    for (u64 length = 0; length < curl.length; length++)
    {
        TEST_EXPECT(http_request_read((str) { curl.data, length }, &request) == HTTP_READ_INCOMPLETE);
    }
    TEST_EXPECT(http_request_read(curl, &request) == HTTP_READ_OK);
    TEST_EXPECT(http_request_read(STR("hello\r\n"), &request) == HTTP_READ_INCOMPLETE);
    TEST_EXPECT(http_request_read(STR("hello\r\n\r\n"), &request) == HTTP_READ_BAD);
}

void http_test_request_line(void)
{
    HttpRequest request = {0};
    TEST_EXPECT(http_request_read(STR("DELETE /a/b?c=d%20e HTTP/1.1\r\nHost: x\r\n\r\n"), &request) == HTTP_READ_OK);
    TEST_EXPECT(str_equal(request.method, STR("DELETE")) && str_equal(request.target, STR("/a/b?c=d%20e")));

    const str bad[] = {
        STR("GET /\r\nHost: x\r\n\r\n"),
        STR("GET / HTTP/1.1 extra\r\nHost: x\r\n\r\n"),
        STR("GET  / HTTP/1.1\r\nHost: x\r\n\r\n"),
        STR(" GET / HTTP/1.1\r\nHost: x\r\n\r\n"),
        STR("GET / HTTP/1.1 \r\nHost: x\r\n\r\n"),
        STR("G(E)T / HTTP/1.1\r\nHost: x\r\n\r\n"),
        STR("GET index.html HTTP/1.1\r\nHost: x\r\n\r\n"),
        STR("GET /a\t/b HTTP/1.1\r\nHost: x\r\n\r\n"),
        STR("GET / HTTP/1.0\r\nHost: x\r\n\r\n"),
        STR("GET / http/1.1\r\nHost: x\r\n\r\n"),
        STR("GET / hTTP/1.1\nHost: x\r\n\r\n"),
        STR("\r\n\r\n"),
    };
    for (u32 i = 0; i < ARRAY_COUNT(bad); i++)
    {
        TEST_EXPECT(http_request_read(bad[i], &request) == HTTP_READ_BAD);
    }
}