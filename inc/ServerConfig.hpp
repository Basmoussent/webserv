#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct LocationConfig {
    std::string path; 
    std::string root; 
    std::vector<std::string> allowed_methods; 
    bool autoindex;
    std::string index_file; 
    std::string redirection; 
    std::string cgi_extension; 
    std::string cgi_path; 
    bool upload_enabled;
    std::string upload_store;
};

struct ServerConfig {
    std::string host; 
    int port;
    std::vector<std::string> server_names; 
    size_t client_max_body_size; 
    std::map<int, std::string> error_pages;
    std::vector<LocationConfig> locations;
};

struct WebServConfig {
    std::vector<ServerConfig> servers; // Liste de tous les serveurs définis dans le fichier de config
};

#endif 