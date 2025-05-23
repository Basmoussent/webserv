int main() {
    ServerManager manager;
    ConfigParser parser(manager);

    // Parser le fichier de configuration
    if (parser.parseFile("config.conf")) {
        // Obtenir un serveur spécifique
        ServerConfig& server = manager.getServer(80);
        
        // Accéder aux paramètres du serveur
        std::string server_name = server.params["server_name"];
        
        // Accéder à une location
        Location& location = server.locations["/"];
        std::string root = location.params["root"];
    }
    return 0;
}