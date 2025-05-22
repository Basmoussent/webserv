#include "../inc/init.hpp"

void initServer1(ServerConfig& server) {
    // Configuration du premier serveur (127.0.0.1:8080)
    server.host = "127.0.0.1";
    server.port = 8080;
    server.server_names.push_back("monsite.local");
    server.server_names.push_back("www.monsite.local");
    server.client_max_body_size = 2 * 1024 * 1024; // 2M en bytes
    
    // Configuration des pages d'erreur
    server.error_pages[404] = "/errors/404.html";
    server.error_pages[500] = "/errors/50x.html";
    server.error_pages[502] = "/errors/50x.html";
    server.error_pages[503] = "/errors/50x.html";
    server.error_pages[504] = "/errors/50x.html";

    // Location "/"
    LocationConfig root_loc;
    root_loc.path = "/";
    root_loc.root = "/var/www/html";
    root_loc.index_file = "index.html index.htm";
    root_loc.autoindex = true;
    root_loc.allowed_methods.push_back("GET");
    root_loc.allowed_methods.push_back("POST");
    server.locations.push_back(root_loc);

    // Location "/redirect-me"
    LocationConfig redirect_loc;
    redirect_loc.path = "/redirect-me";
    redirect_loc.redirection = "https://autresite.com/";
    redirect_loc.autoindex = false;
    redirect_loc.upload_enabled = false;
    server.locations.push_back(redirect_loc);

    // Location "/kapouet"
    LocationConfig kapouet_loc;
    kapouet_loc.path = "/kapouet";
    kapouet_loc.root = "/tmp/www";
    kapouet_loc.autoindex = false;
    kapouet_loc.index_file = "default.html";
    kapouet_loc.allowed_methods.push_back("GET");
    kapouet_loc.upload_enabled = false;
    server.locations.push_back(kapouet_loc);

    // Location "/upload"
    LocationConfig upload_loc;
    upload_loc.path = "/upload";
    upload_loc.root = "/var/www/uploads";
    upload_loc.allowed_methods.push_back("POST");
    upload_loc.upload_enabled = true;
    upload_loc.upload_store = "/var/www/uploads/tmp";
    upload_loc.autoindex = false;
    server.locations.push_back(upload_loc);

    // Location "~ \.php$"
    LocationConfig php_loc;
    php_loc.path = "~ \\.php$";
    php_loc.root = "/var/www/html";
    php_loc.cgi_extension = ".php";
    php_loc.cgi_path = "/run/php/php8.2-fpm.sock";
    php_loc.allowed_methods.push_back("GET");
    php_loc.allowed_methods.push_back("POST");
    php_loc.autoindex = false;
    php_loc.upload_enabled = false;
    server.locations.push_back(php_loc);

    // Location "/errors/"
    LocationConfig errors_loc;
    errors_loc.path = "/errors/";
    errors_loc.root = "/var/www";
    errors_loc.autoindex = false;
    errors_loc.upload_enabled = false;
    server.locations.push_back(errors_loc);
}

void initServer2(ServerConfig& server) {
    // Configuration du second serveur (0.0.0.0:9090)
    server.host = "0.0.0.0";
    server.port = 9090;
    server.error_pages[404] = "/404.html";

    // Location "/"
    LocationConfig root_loc;
    root_loc.path = "/";
    root_loc.root = "/srv/http";
    root_loc.index_file = "index.html";
    root_loc.autoindex = false;
    root_loc.allowed_methods.push_back("GET");
    root_loc.upload_enabled = false;
    server.locations.push_back(root_loc);
}

WebServConfig initializeConfig() {
    WebServConfig config;
    
    // Initialiser le premier serveur
    ServerConfig server1;
    initServer1(server1);
    config.servers.push_back(server1);

    // Initialiser le second serveur
    ServerConfig server2;
    initServer2(server2);
    config.servers.push_back(server2);

    return config;
} 