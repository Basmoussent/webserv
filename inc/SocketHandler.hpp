#pragma once

#include <string>
#include <map>
#include <vector>
#include <sys/socket.h>

struct ServerConfig {
    std::string host;
    int         port;
    // Add other server configuration fields as needed...
};

class SocketHandler {
public:
    SocketHandler();
    ~SocketHandler();

    // Create a listening socket on the given host:port (non-blocking, SO_REUSEADDR).
    // Returns fd≥0 on success, or -1 on failure.
    int createSocket(int port, const std::string& host);

    // Given a list of ServerConfig, open each listening socket and store it in `_serverSockets`.
    bool initServers(const std::vector<ServerConfig>& servers);

    // Return the vector of fds that are currently listening.
    std::vector<int> getServerSockets() const;

private:
    // Map from listening-fd → server config
    std::map<int, ServerConfig> _serverSockets;
};
