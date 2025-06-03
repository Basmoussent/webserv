#pragma once

#include "SocketHandler.hpp"
#include <vector>
#include <poll.h>
#include <algorithm>
#include "Webserv.hpp"
#include <csignal>

class PollManager {
public:
    PollManager(SocketHandler& handler, ConfigParser& configParser);
    ~PollManager();

    bool init();     // add all listening sockets into the internal poll array
    void run();      // enters the infinite poll() → accept/read/write loop

private:
    void addToPoll(int fd, short events);
    void removeFromPoll(int fd);

    SocketHandler& _socketHandler;
    ConfigParser& _configParser;
    std::vector<struct pollfd> _pollfds;
    std::map<int, Handler*> _handlers;
    std::string _buffer;
    
    void cleanupHandlers();
};