#ifndef POLLMANAGER_HPP
#define POLLMANAGER_HPP

#include <vector>
#include <map>
#include <poll.h>
#include "SocketHandler.hpp"
#include "Handler.hpp"
#include "ConfigParser.hpp"

class PollManager {
private:
    SocketHandler& _socketHandler;
    ConfigParser& _configParser;
    std::vector<struct pollfd> _pollfds;
    std::map<int, Handler*> _handlers;

    void removeFromPoll(int fd);
    void cleanupHandlers();

public:
    PollManager(SocketHandler& socketHandler, ConfigParser& configParser);
    ~PollManager();

    bool init();
    void run();
    void addToPoll(int fd, short events);
};

#endif 