#include <iostream>
#include <string>
#include "Request.hpp"
#include "Handler.hpp"
#include "ConfigParser.hpp"

void runTests(ConfigParser& parser) {
    const int NUM_REQUESTS = 5;
    std::string requests[NUM_REQUESTS];
    
    requests[0] = "GET /log?user=user HTTP/1.1\r\n"
                  "Host: 127.0.0.1\r\n"
                  "\r\n";

    requests[1] = "POST /test HTTP/1.1\r\n"
                  "Host: 127.0.0.1\r\n"
                  "Content-Type: application/json\r\n"
                  "Content-Length: 52\r\n"
                  "\r\n"
                  "{\r\n"
                  "  \"username\": \"alice\",\r\n"
                  "  \"password\": \"qwe\"\r\n"
                  "}\r\n";

    requests[2] = "DELETE /test HTTP/1.1\r\n"
                  "Host: 127.0.0.1\r\n"
                  "\r\n"
                  "post_20250523_200239.log";
    
    requests[3] = "POST /upload HTTP/1.1\r\n"
                  "Host: 127.0.0.1\r\n"
                  "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                  "Content-Length: 267\r\n"
                  "\r\n"
                  "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                  "Content-Disposition: form-data; name=\"file\"; filename=\"example.txt\"\r\n"
                  "Content-Type: text/plain\r\n"
                  "\r\n"
                  "Hello, this is the content of the uploaded file.\r\n"
                  "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";
    
    requests[4] = "GET /cgi-bin HTTP/1.1\r\n"
                  "Host: 127.0.0.1\r\n"
                  "\r\n"
                  "test.sh";

    // Test each request
    std::cout << "\n=== Testing Requests ===" << std::endl;
    for (int i = 0; i < NUM_REQUESTS; ++i) {
        std::cout << "\nProcessing Request " << i + 1 << ":" << std::endl;
        std::cout << "-------------------" << std::endl;
        std::cout << requests[i] << std::endl;

        try {
            Request request(requests[i]);
            Handler handler(request, parser);
            std::cout << "\nResponse:" << std::endl;
            std::cout << "-------------------" << std::endl;
            std::cout << handler.getResponse() << std::endl;
        }
        catch (std::exception& e) {
            std::cerr << "Error processing request: " << e.what() << std::endl;
        }
    }
} 