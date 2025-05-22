#ifndef INIT_HPP
#define INIT_HPP

#include "ServerConfig.hpp"

// Fonction principale qui initialise toute la configuration
WebServConfig initializeConfig();

// Fonctions d'initialisation individuelles pour chaque serveur
void initServer1(ServerConfig& server);
void initServer2(ServerConfig& server);

#endif 