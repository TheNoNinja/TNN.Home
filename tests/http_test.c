#include "test.h"
#include "http/http.h"

void http_test_request(void)
{
    const str curl = STR(
        "GET / HTTP/1.1\r\n"
        "Host: 127.0.0.1:8080\r\n"
        "User-Agent: curl/8.18.0\r\n"
        "Accept: */*\r\n"
        "\r\n"
    );
    HttpRequest request = {0};
    TEST_EXPECT(http_request_read(curl, &request) == HTTP_READ_OK);
    TEST_EXPECT(str_equal(request.method, STR("GET")) && str_equal(request.target, STR("/")));
    TEST_EXPECT(str_equal(request.host, STR("127.0.0.1:8080")));
    TEST_EXPECT(request.content_length == 0 && request.head_length == curl.length);
    TEST_EXPECT(request.method.data == curl.data && request.target.data == curl.data + 4);

    const str post = STR(
        "POST /set HTTP/1.1\r\n"
        "Host: 127.0.0.1:8080\r\n"
        "Content-Length: 23\r\n"
        "\r\n"
        "tnn-sim-1 light/on true"
    );
    TEST_EXPECT(http_request_read(post, &request) == HTTP_READ_OK);
    TEST_EXPECT(str_equal(request.method, STR("POST")) && str_equal(request.target, STR("/set")));
    TEST_EXPECT(request.content_length == 23 && request.head_length + 23 == post.length);
}

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
        STR("GET /caf\xC3\xA9 HTTP/1.1\r\nHost: x\r\n\r\n"),
        STR("\r\n\r\n"),
    };
    for (u32 i = 0; i < ARRAY_COUNT(bad); i++)
    {
        TEST_EXPECT(http_request_read(bad[i], &request) == HTTP_READ_BAD);
    }
}

void http_test_request_headers(void)
{
    HttpRequest request = {0};
    const str loose = STR(
        "GET / HTTP/1.1\r\n"
        "X-Anything: a value, with (signs) and \t a tab\r\n"
        "hOsT:example.org  \r\n"
        "CONTENT-LENGTH: \t 5\r\n"
        "\r\n"
        "hello"
    );
    TEST_EXPECT(http_request_read(loose, &request) == HTTP_READ_OK);
    TEST_EXPECT(str_equal(request.host, STR("example.org")) && request.content_length == 5);

    const str bad[] = {
        STR("GET / HTTP/1.1\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nAccept: */*\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost: x\r\nHost: x\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost:\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost: a b\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost : x\r\n\r\n"),
        STR("GET / HTTP/1.1\r\n Host: x\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost x\r\n\r\n"),
        STR("GET / HTTP/1.1\r\n: x\r\nHost: x\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost: x\r\nX: a\rb\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost: x\r\nX: a\x01" "b\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost: x\r\nContent-Length: five\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost: x\r\nContent-Length: -5\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\nContent-Length: 5\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost: x\r\nContent-Length: 18446744073709551616\r\n\r\n"),
        STR("GET / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n"),
    };
    for (u32 i = 0; i < ARRAY_COUNT(bad); i++)
    {
        TEST_EXPECT(http_request_read(bad[i], &request) == HTTP_READ_BAD);
    }
}