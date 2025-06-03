#include "Webserv.hpp"
#include <iostream>
#include <cstdlib>

void runServer(ConfigParser& parser) {
    std::vector<ServerConfig> servers;
    const std::vector<Server>& configServers = parser.getServers();
    
    for (size_t i = 0; i < configServers.size(); ++i) {
        const Server& srv = configServers[i];
        ServerConfig config;
        
        std::map<std::string, std::string>::const_iterator it;
        
        it = srv.instruct.find("host");
        if (it != srv.instruct.end()) {
            config.host = it->second;
        } else {
            config.host = "127.0.0.1";
        }
        
        it = srv.instruct.find("listen");
        if (it != srv.instruct.end()) {
            config.port = std::atoi(it->second.c_str());
        } else {
            config.port = 8080;
        }
        
        servers.push_back(config);
    }
    
    SocketHandler socketHandler;
    if (!socketHandler.initServers(servers)) {
        std::cerr << "Erreur lors de l'initialisation des sockets serveur" << std::endl;
        return;
    }
    
    PollManager pollManager(socketHandler, parser);
    
    if (!pollManager.init()) {
        std::cerr << "Erreur lors de l'initialisation du PollManager" << std::endl;
        return;
    }
    
    std::cout << "Serveur démarré, en attente de connexions..." << std::endl;
    pollManager.run();
} 