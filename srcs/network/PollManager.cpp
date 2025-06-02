#include "PollManager.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>  // Pour sockaddr_in

// Helper pour mettre un fd en non-bloquant
static void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

PollManager::PollManager(SocketHandler& socketHandler, ConfigParser& configParser)
    : _socketHandler(socketHandler), _configParser(configParser) {
    const std::vector<int>& sockets = _socketHandler.getServerSockets();
    for (std::vector<int>::const_iterator it = sockets.begin(); it != sockets.end(); ++it) {
        pollfd pfd;
        pfd.fd = *it;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
    }
}

PollManager::~PollManager() {
    cleanupHandlers();
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        close(_pollfds[i].fd);
    }
}

void PollManager::cleanupHandlers() {
    for (std::map<int, Handler*>::iterator it = _handlers.begin(); it != _handlers.end(); ++it) {
        delete it->second;
    }
    _handlers.clear();
}

bool PollManager::init() {
    std::vector<int> server_socks = _socketHandler.getServerSockets();
    if (server_socks.empty()) {
        write(2, "[ERROR] Aucun socket serveur à initialiser\n", 44);
        return false;
    }

    write(1, "[DEBUG] Initialisation de la boucle poll\n", 41);
    for (size_t i = 0; i < server_socks.size(); ++i) {
        struct pollfd pfd;
        pfd.fd      = server_socks[i];
        pfd.events  = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
        write(1, "[DEBUG] Serveur ajouté à poll\n", 30);
    }
    return true;
}
int code = 0;

void stop_handler(int signum) {
    if (signum == SIGINT) {
        write(1, "[DEBUG] Arrêt du serveur demandé\n", 35);
        code = 1;
    }
}

void PollManager::run() {
    std::map<int, std::string> clientBuffers;
    signal(SIGINT, stop_handler);
    while (code == 0) {
        int ready = poll(&_pollfds[0], _pollfds.size(), -1);
        if (ready < 0) {
            write(2, "[ERROR] poll() failed\n", 22);
            continue;
        }

        std::vector<int> to_remove;

        for (int idx = static_cast<int>(_pollfds.size()) - 1; idx >= 0; --idx) {
            struct pollfd& pfd = _pollfds[idx];

            if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
                write(1, "[DEBUG] Erreur ou déconnexion détectée\n", 40);
                to_remove.push_back(pfd.fd);
                continue;
            }

            if (pfd.revents & POLLIN) {
                std::vector<int> servers = _socketHandler.getServerSockets();
                bool is_server = (std::find(servers.begin(),
                                            servers.end(),
                                            pfd.fd) != servers.end());

                if (is_server) {
                    // Nouvelle connexion entrante
                    write(1, "[DEBUG] Accept sur socket serveur\n", 35);

                    struct sockaddr_in cli_addr;
                    socklen_t cli_len = sizeof(cli_addr);
                    int client_fd = accept(pfd.fd,
                                           (struct sockaddr*)&cli_addr,
                                           &cli_len);
                    if (client_fd < 0) {
                        write(2, "[ERROR] accept() < 0, abandon connexion\n", 40);
                        continue;
                    }

                    setNonBlocking(client_fd);

                    // Ajouter le client à poll
                    struct pollfd new_pfd;
                    new_pfd.fd      = client_fd;
                    new_pfd.events  = POLLIN;
                    new_pfd.revents = 0;
                    _pollfds.push_back(new_pfd);
                    
                    // Initialiser le buffer pour ce client
                    clientBuffers[client_fd] = "";
                    write(1, "[DEBUG] Client FD ajouté à poll\n", 33);
                }
                else {
                    // Données à lire sur un client existant
                    write(1, "[DEBUG] Données dispo sur socket client\n", 41);

                    char buffer[1024];
                    int bytes = recv(pfd.fd, buffer, sizeof(buffer) - 1, 0);
                    if (bytes <= 0) {
                        write(1, "[DEBUG] Fermeture du client détectée\n", 38);
                        clientBuffers.erase(pfd.fd);
                        to_remove.push_back(pfd.fd);
                        continue;
                    }

                    buffer[bytes] = '\0';
                    
                    // Accumuler les données dans le buffer du client
                    if (clientBuffers.find(pfd.fd) == clientBuffers.end()) {
                        clientBuffers[pfd.fd] = "";
                    }
                    clientBuffers[pfd.fd] += std::string(buffer, bytes);

                    write(1, "[DEBUG] Données reçues et accumulées\n", 38);

                    // Vérifier si on a une requête HTTP complète
                    std::string& fullBuffer = clientBuffers[pfd.fd];
                    if (fullBuffer.find("\r\n\r\n") != std::string::npos) {
                        write(1, "[DEBUG] Requête HTTP complète détectée\n", 40);
                        
                        try {
                            Request request(fullBuffer);
                            
                            if (request.isValid()) {
                                write(1, "[DEBUG] Requête valide, création du Handler\n", 45);
                                
                                Handler* handler = new Handler(request, _configParser);
                                _handlers[pfd.fd] = handler;
                                
                                std::string response = handler->getResponse();
                                
                                int sent = send(pfd.fd, response.c_str(), response.length(), 0);
                                if (sent < 0) {
                                    write(2, "[ERROR] Envoi de la réponse échoué\n", 36);
                                    to_remove.push_back(pfd.fd);
                                } else {
                                    write(1, "[DEBUG] Réponse envoyée avec succès\n", 39);
                                    
                                    std::string connection = request.getHeader("Connection");
                                    if (connection == "close") {
                                        write(1, "[DEBUG] Client demande la fermeture de la connexion\n", 52);
                                        to_remove.push_back(pfd.fd);
                                    } else {
                                        clientBuffers[pfd.fd] = "";
                                    }
                                }
                                delete handler;
                                _handlers.erase(pfd.fd);
                            } else {
                                write(1, "[DEBUG] Requête invalide\n", 26);
                                std::string errorResponse = "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\nConnection: close\r\n\r\nBad Request";
                                send(pfd.fd, errorResponse.c_str(), errorResponse.length(), 0);
                                to_remove.push_back(pfd.fd);
                            }
                        } catch (...) {
                            write(2, "[ERROR] Exception lors du traitement de la requête\n", 53);
                            // Envoyer une erreur 500
                            std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\nConnection: close\r\n\r\nInternal Server Error";
                            send(pfd.fd, errorResponse.c_str(), errorResponse.length(), 0);
                            to_remove.push_back(pfd.fd);
                        }
                        
                        if (std::find(to_remove.begin(), to_remove.end(), pfd.fd) != to_remove.end()) {
                            clientBuffers.erase(pfd.fd);
                        }
                    }
                }
            }

            pfd.revents = 0;
        }

        // Enlever définitivement les FDs marquées
        for (size_t i = 0; i < to_remove.size(); ++i) {
            removeFromPoll(to_remove[i]);
        }
    }
    
    write(1, "[DEBUG] Nettoyage des ressources...\n", 36);
    
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        if (_pollfds[i].fd > 2) {
            close(_pollfds[i].fd);
        }
    }
    std::vector<int> server_socks = _socketHandler.getServerSockets();
    for (size_t i = 0; i < server_socks.size(); i++) {
        close(server_socks[i]);
    }
    cleanupHandlers();
    write(1, "[DEBUG] Arrêt du serveur terminé\n", 34);
}

void PollManager::addToPoll(int fd, short events) {
    struct pollfd newfd;
    newfd.fd      = fd;
    newfd.events  = events;
    newfd.revents = 0;
    _pollfds.push_back(newfd);
    char buf[64];
    snprintf(buf, sizeof(buf), "[DEBUG] Ajout FD %d au poll\n", fd);
    write(1, buf, strlen(buf));
}

void PollManager::removeFromPoll(int fd) {
    std::map<int, Handler*>::iterator handler_it = _handlers.find(fd);
    if (handler_it != _handlers.end()) {
        delete handler_it->second;
        _handlers.erase(handler_it);
    }
    
    for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
        if (it->fd == fd) {
            _pollfds.erase(it);
            break;
        }
    }
}
