#include <iostream>
#include <string>
#include <vector>
#include <poll.h>       // poll() for multiplexing multiple sockets
#include <unistd.h>     // read(), write(), close()
#include <sys/socket.h> // socket(), bind(), listen(), accept()
#include <netinet/in.h> // sockaddr_in, htons(), INADDR_ANY
#include <arpa/inet.h>  // htonl(), htons(), inet_pton()
#include "inc/ServerConfig.hpp"
#include "inc/init.hpp"

// Function to send HTTP response
void sendResponse(int client_socket, const std::string& content) {
    std::string response = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html\r\n"
                          "Content-Length: " + std::to_string(content.length()) + "\r\n"
                          "\r\n" + content;
    send(client_socket, response.c_str(), response.length(), 0);
}

// Function to setup a server socket
int setupServerSocket(const ServerConfig& config) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket for " << config.host << ":" << config.port << std::endl;
        return -1;
    }

    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options for " << config.host << ":" << config.port << std::endl;
        close(server_fd);
        return -1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(config.host.c_str());
    address.sin_port = htons(config.port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to " << config.host << ":" << config.port << std::endl;
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Failed to listen on " << config.host << ":" << config.port << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Server listening on " << config.host << ":" << config.port << std::endl;
    return server_fd;
}

int main() {
    // Initialiser la configuration avec les serveurs hard-assigné dans src/init.cpp
    WebServConfig config = initializeConfig();

    if (config.servers.empty()) {
        std::cerr << "No server configurations found" << std::endl;
        return 1;
    }

    // Setup server sockets, capable de gérer plusieurs serveurs par la loop for
    std::vector<int> server_fds;
    for (size_t i = 0; i < config.servers.size(); ++i) {
        int server_fd = setupServerSocket(config.servers[i]);
        if (server_fd >= 0) {
            server_fds.push_back(server_fd);
        }
    }

    if (server_fds.empty()) {
        std::cerr << "No server sockets could be created" << std::endl;
        return 1;
    }

    // Setup poll structure
    std::vector<pollfd> poll_fds;
    for (size_t i = 0; i < server_fds.size(); ++i) {
        pollfd pfd;
        pfd.fd = server_fds[i];
        pfd.events = POLLIN;
        pfd.revents = 0;
        poll_fds.push_back(pfd);
    }

    while (true) {
        // Wait for activity on any socket
        if (poll(poll_fds.data(), poll_fds.size(), -1) < 0) {
            std::cerr << "Poll error" << std::endl;
            continue;
        }

        // Check each server socket
        for (size_t i = 0; i < poll_fds.size(); ++i) {
            if (poll_fds[i].revents & POLLIN) {
                int client_socket = accept(poll_fds[i].fd, NULL, NULL);
                if (client_socket < 0) {
                    std::cerr << "Failed to accept connection on server " << i << std::endl;
                    continue;
                }

                char buffer[1024] = {0};
                recv(client_socket, buffer, 1024, 0);
                
                // Print the raw request with separated request line and headers
                std::cout << "Request received on " << config.servers[i].host << ":" 
                         << config.servers[i].port << ":\n"
                         << "Request line: " << strtok(buffer, "\n") << "\n"
                         << "Headers:\n" << strtok(NULL, "") << std::endl;
                
                // print a simple work in progress message
                std::string content = "<html><body>"
                                    "<h1>Work in Progress</h1>"
                                    "<p>Request received on server: " + 
                                    config.servers[i].host + ":" + 
                                    std::to_string(config.servers[i].port) + "</p>"
                                    "<p>Request parsing and handling will be implemented later.</p>"
                                    "</body></html>";
                
                sendResponse(client_socket, content);
                close(client_socket);
            }
        }
    }

    // Cleanup (need signal handling to reach this line)
    for (size_t i = 0; i < server_fds.size(); ++i) {
        close(server_fds[i]);
    }

    return 0;
}
