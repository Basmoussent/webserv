#pragma once

#include <vector>
#include <poll.h>
#include <map>
#include "SocketHandler.hpp"
#include "Handler.hpp"
#include "ConfigParser.hpp"

class PollManager {
public:
    PollManager(SocketHandler& socketHandler, ConfigParser& configParser);
    ~PollManager();

    bool init();     // add all listening sockets into the internal poll array
    void run();      // enters the infinite poll() → accept/read/write loop

private:
    void addToPoll(int fd, short events);
    void removeFromPoll(int fd);

    void handleNewConnection(int server_fd);
    void handleClientData(int client_fd);
    void cleanupHandlers();

    std::map<int, Handler> _handlers;
    std::vector<struct pollfd> _pollfds;
    SocketHandler& _socketHandler;
    ConfigParser& _configParser;
};
