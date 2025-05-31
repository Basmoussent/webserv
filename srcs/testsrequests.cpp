#include "Request.hpp"
#include <iostream>
#include <string>

void runTest() {
    std::cout << "Running request tests..." << std::endl;
    
    // Test 1: Basic GET request
    std::string getRequest = "GET / HTTP/1.1\r\n"
                            "Host: localhost:8080\r\n"
                            "Connection: keep-alive\r\n\r\n";
    Request req1(getRequest);
    std::cout << "Test 1 - GET Request:" << std::endl;
    std::cout << req1 << std::endl;

    // Test 2: POST request with chunked encoding
    std::string postRequest = "POST /upload HTTP/1.1\r\n"
                             "Host: localhost:8080\r\n"
                             "Transfer-Encoding: chunked\r\n"
                             "Content-Type: text/plain\r\n\r\n"
                             "5\r\n"
                             "Hello\r\n"
                             "6\r\n"
                             "World!\r\n"
                             "0\r\n\r\n";
    Request req2(postRequest);
    std::cout << "Test 2 - POST Request with chunked encoding:" << std::endl;
    std::cout << req2 << std::endl;

    // Test 3: Request with query string
    std::string queryRequest = "GET /search?q=test&page=1 HTTP/1.1\r\n"
                              "Host: localhost:8080\r\n\r\n";
    Request req3(queryRequest);
    std::cout << "Test 3 - Request with query string:" << std::endl;
    std::cout << req3 << std::endl;
} 