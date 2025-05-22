#include <iostream>
#include <string>
#include "Request.hpp"
#include "Handler.hpp"

int main() {
    std::string raw_request =
        "POST /log HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: curl/7.81.0\r\n"
        "Accept: */*\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 52\r\n"
        "Connection: keep-alive\r\n"
        "Authorization: Bearer abcdef123456\r\n"
        "Cache-Control: no-cache\r\n"
        "X-Custom-Header: some-value\r\n"
        "\r\n"
        "{\r\n"
        "  \"username\": \"alice\",\r\n"
        "  \"password\": \"secret123\"\r\n"
        "}\r\n";

    Request req(raw_request);

    std::cout << req << std::endl;

    Handler handl(req);

    std::cout << handl << std::endl;

    return 0;
}