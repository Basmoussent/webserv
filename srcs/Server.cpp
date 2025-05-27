#include <iostream>
#include <string>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include "ConfigParser.hpp"
#include "Request.hpp"
#include "Handler.hpp"

// Function to print socket information
void printSocketInfo(int sock, const char* prefix) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (getsockname(sock, (struct sockaddr*)&addr, &len) == 0) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
        std::cout << prefix << " Socket " << sock << " bound to " << ip << ":" << ntohs(addr.sin_port) << std::endl;
    } else {
        std::cout << prefix << " Socket " << sock << " (getsockname failed: " << strerror(errno) << ")" << std::endl;
    }
}

// Function to set socket to non-blocking mode
void setNonBlocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        std::cerr << "Failed to get socket flags: " << strerror(errno) << std::endl;
        return;
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "Failed to set socket to non-blocking: " << strerror(errno) << std::endl;
    }
}

// Function to send HTTP response
void sendResponse(int client_socket, const std::string& content)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: text/html\r\n"
        << "Content-Length: " << content.length() << "\r\n"
        << "\r\n" << content;

    std::string response = oss.str();
    std::cout << "Sending response:\n" << response << std::endl;
    ssize_t sent = send(client_socket, response.c_str(), response.length(), 0);
    if (sent < 0) {
        std::cerr << "Failed to send response: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Sent " << sent << " bytes" << std::endl;
    }
}

// Function to setup a server socket
int setupServerSocket(const Server& server) {
    std::cout << "\nSetting up server socket..." << std::endl;
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return -1;
    }
    std::cout << "Created socket " << server_fd << std::endl;

    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "Set SO_REUSEADDR option" << std::endl;

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    std::map<std::string, std::string>::const_iterator host_it = server.instruct.find("host");
    std::map<std::string, std::string>::const_iterator port_it = server.instruct.find("listen");
    
    // Convert host string to IP address using inet_pton
    if (inet_pton(AF_INET, host_it->second.c_str(), &address.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << host_it->second << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "Converted IP address: " << host_it->second << std::endl;
    
    address.sin_port = htons(atoi(port_it->second.c_str()));
    std::cout << "Converted port: " << port_it->second << std::endl;

    std::cout << "Attempting to bind to " << host_it->second << ":" << port_it->second << std::endl;

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind: " << strerror(errno) << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "Successfully bound socket" << std::endl;
    printSocketInfo(server_fd, "Server");

    if (listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "Server listening on " << host_it->second << ":" << port_it->second << std::endl;
    
    return server_fd;
}

void runServer(ConfigParser& parser) {
    std::cout << "\nInitializing server..." << std::endl;
    
    // Setup server sockets for each server in configuration
    std::vector<int> server_fds;
    const std::vector<Server>& servers = parser.getServers();
    for (size_t i = 0; i < servers.size(); ++i) {
        std::cout << "\nSetting up server " << i + 1 << std::endl;
        int server_fd = setupServerSocket(servers[i]);
        if (server_fd >= 0) {
            server_fds.push_back(server_fd);
            setNonBlocking(server_fd);
        }
    }

    if (server_fds.empty()) {
        std::cerr << "No server sockets could be created" << std::endl;
        return;
    }

    // Setup poll structure
    std::vector<pollfd> poll_fds;
    for (size_t i = 0; i < server_fds.size(); ++i) {
        pollfd pfd;
        pfd.fd = server_fds[i];
        pfd.events = POLLIN;
        pfd.revents = 0;
        poll_fds.push_back(pfd);
        std::cout << "Added server socket " << server_fds[i] << " to poll" << std::endl;
    }

    std::cout << "\n=== Server Started ===" << std::endl;
    while (true) {
        std::cout << "\nWaiting for connections..." << std::endl;
        
        // Wait for activity on any socket
        int poll_result = poll(poll_fds.data(), poll_fds.size(), -1);
        if (poll_result < 0) {
            std::cerr << "Poll error: " << strerror(errno) << std::endl;
            continue;
        }
        
        std::cout << "Poll returned: " << poll_result << " events" << std::endl;

        // Check each server socket
        for (size_t i = 0; i < poll_fds.size(); ++i) {
            if (poll_fds[i].revents & POLLIN) {
                std::cout << "\nActivity detected on server " << i << std::endl;
                
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                memset(&client_addr, 0, client_len);
                
                int client_socket = accept(poll_fds[i].fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_socket < 0) {
                    std::cerr << "Failed to accept connection on server " << i << ": " << strerror(errno) << std::endl;
                    continue;
                }
                
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
                std::cout << "Accepted connection from " << client_ip << ":" << ntohs(client_addr.sin_port) 
                         << " on socket " << client_socket << std::endl;
                
                printSocketInfo(client_socket, "Client");

                char buffer[1024] = {0};
                ssize_t bytes_received = recv(client_socket, buffer, 1024, 0);
                if (bytes_received <= 0) {
                    std::cerr << "Failed to receive data from client: " << strerror(errno) << std::endl;
                    close(client_socket);
                    continue;
                }
                std::cout << "Received " << bytes_received << " bytes" << std::endl;

                // Create Request object and parse the received data
                std::string raw_request(buffer, bytes_received);
                Request request(raw_request);
                
                // Create Handler to process the request
                Handler handler(request, parser);

                // Send the response back to the client
                std::string response = handler.getResponse();
                ssize_t sent = send(client_socket, response.c_str(), response.length(), 0);
                if (sent < 0) {
                    std::cerr << "Failed to send response: " << strerror(errno) << std::endl;
                } else {
                    std::cout << "Sent " << sent << " bytes" << std::endl;
                    std::cout << "Response sent:\n" << response << std::endl;
                }

                close(client_socket);
                std::cout << "Connection closed" << std::endl;
            }
        }
    }

    // Cleanup (need signal handling to reach this line)
    for (size_t i = 0; i < server_fds.size(); ++i) {
        close(server_fds[i]);
    }
} 