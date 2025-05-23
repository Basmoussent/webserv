#include <iostream>
#include <string>
#include "Request.hpp"
#include "Handler.hpp"

int main(int ac, char **av) {
    const int NUM_REQUESTS = 3;
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
                  "post_20250523_195301.log";
                
    ConfigParser parser;

    if (ac < 2) {
        std::cerr << "Usage: ./a.out <config_file>" << std::endl;
        return 1;
    }

    // Parse configuration
    if (!parser.parseFile(av[1])) {
        std::cerr << "Échec du parsing du fichier de configuration." << std::endl;
        return 1;
    }

    std::cout << "\n=== Testing Configuration ===" << std::endl;
    parser.printServers();

    // Test each request
    std::cout << "\n=== Testing Requests ===" << std::endl;
    for (int i = 0; i < NUM_REQUESTS; ++i) {
        std::cout << "\nProcessing Request:" << std::endl;
        std::cout << "-------------------" << std::endl;
        std::cout << requests[i] << std::endl;

        try {
            Request req(requests[i]);
            Handler handler(req, parser);
            
            // Access configuration data
            const std::vector<Server>& servers = parser.getServers();
            if (servers.size() > 0) {
            }
        }
        catch (std::exception& e) {
            std::cerr << "Error processing request: " << e.what() << std::endl;
        }
    }

    return 0;
}