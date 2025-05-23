// Structure pour une location
struct Location {
    std::string path;
    std::map<std::string, std::string> params;
};

// Structure pour un serveur
struct ServerConfig {
    std::map<std::string, std::string> params;
    std::map<std::string, Location> locations;
};

// Classe principale pour gérer les serveurs
class ServerManager {
private:
    std::map<int, ServerConfig> servers;  // port -> ServerConfig
    int default_port;

public:
    // Obtenir un serveur par son port
    ServerConfig& getServer(int port) {
        auto it = servers.find(port);
        if (it != servers.end()) {
            return it->second;
        }
        return servers[default_port];
    }

    // Ajouter un serveur
    void addServer(int port, const ServerConfig& config) {
        servers[port] = config;
    }

    // Définir le serveur par défaut
    void setDefaultPort(int port) {
        default_port = port;
    }
};

// Classe pour parser le fichier de configuration
class ConfigParser {
};