#include "ConfigParser.hpp"
#include "SocketHandler.hpp"
#include "PollManager.hpp"
#include "Handler.hpp"

int main(int ac, char **av)
{
    if (ac < 2)
    {
        write(2, "Usage: ./webserv <config_file>\n", 28);
        return 1;
    }
    
    // 1. Parser la config
    ConfigParser parser;
    if (!parser.parseFile(av[1]))
    {
        write(2, "Configuration file parsing failed.\n", 34);
        return 1;
    }
    if (!parser.validateConfig())
    {
        write(2, "Configuration file validation failed.\n", 36);
        return 1;
    }

    // 2. Initialiser les sockets
    SocketHandler handler;
	std::vector<Server> servers = parser.getServers();
	std::vector<ServerConfig> serverConfigs;

	// Convertir les Server en ServerConfig
	for (size_t i = 0; i < servers.size(); ++i) {
		ServerConfig config;
		config.host = servers[i].instruct["host"];
		config.port = atoi(servers[i].instruct["listen"].c_str());
		serverConfigs.push_back(config);
	}

	if (!handler.initServers(serverConfigs)) {
		write(2, "Failed to initialize servers\n", 29);
		return 1;
	}
    
    // 3. Initialiser poll
    PollManager pollManager(handler, parser);
    if (!pollManager.init())
    {
        write(2, "Failed to initialize poll\n", 26);
        return 1;
    }

    write(1, "Server initialized and ready\n", 29);
    write(1, "Starting server...\n", 19);
    
    // 4. Démarrer le serveur
    pollManager.run();
    
    return 0;
}