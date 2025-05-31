#pragma once

#include "SocketHandler.hpp"
#include <vector>
#include <poll.h>

class PollManager {
public:
    PollManager(SocketHandler& handler);
    ~PollManager();

    bool init();     // add all listening sockets into the internal poll array
    void run();      // enters the infinite poll() → accept/read/write loop

private:
    void addToPoll(int fd, short events);
    void removeFromPoll(int fd);

    void handleNewConnection(int server_fd);
    void handleClientData(int client_fd);

    SocketHandler& _socketHandler;
    std::vector<struct pollfd> _pollfds;
};
