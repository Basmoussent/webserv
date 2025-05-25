#include <iostream>
#include "ConfigParser.hpp"
#include "Server.hpp"
#include "TestRequests.hpp"

int main(int ac, char **av) {
    if (ac < 2) {
        std::cerr << "Usage: ./webserv <config_file> [--test]" << std::endl;
        return 1;
    }

    // Parse configuration
    ConfigParser parser;
    if (!parser.parseFile(av[1])) {
        std::cerr << "Échec du parsing du fichier de configuration." << std::endl;
        return 1;
    }

    std::cout << "\n=== Testing Configuration ===" << std::endl;
    parser.printServers();

    // Check if we're in test mode
    if (ac > 2 && std::string(av[2]) == "--test") {
        runTests(parser);
        return 0;
    }

    // Run the server
    runServer(parser);

    return 0;
}