#include "Webserv.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/resource.h>

SocketHandler::SocketHandler() {
}

SocketHandler::~SocketHandler() {}

int SocketHandler::createSocket(int port, const std::string& host) {
    // 1) Make an IPv4 TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        return -1;
    }

    // 2) Enable SO_REUSEADDR
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        close(sockfd);
        return -1;
    }

    // 3) Build sockaddr_in
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    // If host is a dotted-quad, use inet_pton. Otherwise bind to INADDR_ANY.
    struct in_addr inaddr4;
    if (inet_pton(AF_INET, host.c_str(), &inaddr4) == 1) {
        addr.sin_addr = inaddr4;
    } else {
        // Fallback: bind to ANY
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    // 4) bind()
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return -1;
    }

    // 5) listen()
    if (listen(sockfd, SOMAXCONN) < 0) {
        perror("listen()");
        close(sockfd);
        return -1;
    }

    // 6) Make the socket non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL)");
        close(sockfd);
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL, O_NONBLOCK)");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

bool SocketHandler::initServers(const std::vector<ServerConfig>& servers) {
    for (size_t i = 0; i < servers.size(); ++i) {
        int port = servers[i].port;
        std::string host = servers[i].host;

        int sockfd = createSocket(port, host);
        if (sockfd < 0) {
            write(2, "[ERROR] Could not create socket\n", 33);
            return false;
        }

        // Store the listening FD → config mapping
        _serverSockets[sockfd] = servers[i];
    }
    return true;
}

std::vector<int> SocketHandler::getServerSockets() const {
    std::vector<int> out;
    out.reserve(_serverSockets.size());
    for (std::map<int, ServerConfig>::const_iterator it = _serverSockets.begin();
         it != _serverSockets.end(); ++it) {
        out.push_back(it->first);
    }
    return out;
}
