#include "PollManager.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <algorithm>
#include "Handler.hpp"
#include "Request.hpp"
#include "ConfigParser.hpp"
#include <utility>

#define MESSAGE_BUFFER 4096

// Helper pour mettre un fd en non-bloquant
static void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

PollManager::PollManager(SocketHandler& socketHandler, ConfigParser& configParser)
    : _socketHandler(socketHandler), _configParser(configParser) {}

PollManager::~PollManager() {
    cleanupHandlers();
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        close(_pollfds[i].fd);
    }
}

void PollManager::cleanupHandlers() {
    for (std::map<int, Handler>::iterator it = _handlers.begin(); it != _handlers.end(); ++it) {
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

void PollManager::run() {
    write(1, "[DEBUG] Démarrage boucle poll\n", 30);
    while (true) {
        if (_pollfds.empty()) {
            write(2, "[ERROR] Aucun FD dans le poll, arrêt.\n", 38);
            break;
        }

        int ret = poll(_pollfds.data(), _pollfds.size(), -1);
        if (ret < 0) {
            write(2, "[ERROR] poll() a échoué, arrêt du serveur\n", 42);
            break;
        }
        write(1, "[DEBUG] poll() a renvoyé un événement\n", 39);

        // Stocker les FDs à supprimer après avoir parcouru tout le tableau
        std::vector<int> to_remove;

        // Parcours EN ARRIÈRE pour pouvoir effacer sans casser l'itération
        for (int idx = static_cast<int>(_pollfds.size()) - 1; idx >= 0; --idx) {
            struct pollfd &pfd = _pollfds[idx];

            // 1) Si poll() dit qu'il y a une erreur ou fermeture
            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                char buf[64];
                snprintf(buf, sizeof(buf),
                         "[ERROR] FD %d invalide (revents=0x%x). Suppression.\n",
                         pfd.fd, pfd.revents);
                write(2, buf, strlen(buf));
                to_remove.push_back(pfd.fd);
                continue;
            }

            // 2) Si c'est POLLIN : on distingue serveur ou client
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

                    // Mettre client en non-bloquant
                    setNonBlocking(client_fd);

                    // On ajoute la socket client à poll pour la prochaine itération
                    struct pollfd new_pfd;
                    new_pfd.fd      = client_fd;
                    new_pfd.events  = POLLIN;
                    new_pfd.revents = 0;
                    _pollfds.push_back(new_pfd);
                    write(1, "[DEBUG] Client FD ajouté à poll\n", 33);
                }
                else {
                    // On a des données à lire sur un client existant
                    write(1, "[DEBUG] Données dispo sur socket client\n", 41);

                    std::map<int, Handler>::iterator handler_it = _handlers.find(pfd.fd);
                    if (handler_it == _handlers.end()) {
                        // Nouvelle requête
                        char buffer[MESSAGE_BUFFER];
                        memset(buffer, 0, sizeof(buffer));
                        ssize_t bytes_read = read(pfd.fd, buffer, sizeof(buffer) - 1);
                        
                        if (bytes_read == 0) {
                            write(2, "[ERROR] Connexion fermée par le client\n", 41);
                            to_remove.push_back(pfd.fd);
                            continue;
                        }
                        else if (bytes_read < 0) {
                            write(2, "[ERROR] Erreur de lecture\n", 26);
                            to_remove.push_back(pfd.fd);
                            continue;
                        }
                        else if (bytes_read > 0) {
                            Request request;
                            request.feed(buffer, bytes_read);
                            Handler handler(request, _configParser);
                            _handlers.insert(std::make_pair(pfd.fd, handler));

                            // Si la requête n'est pas complète, continuer à lire
                            if (!request.isComplete()) {
                                pfd.events = POLLIN;
                                continue;
                            }

                            // Traiter la requête complète
                            handler.process();

                            // Envoyer la réponse
                            std::string response = handler.getResponse();
                            write(pfd.fd, response.c_str(), response.length());

                            // Marquer pour suppression
                            to_remove.push_back(pfd.fd);
                        }
                    } else {
                        // Continuer la lecture de la requête existante
                        char buffer[MESSAGE_BUFFER];
                        memset(buffer, 0, sizeof(buffer));
                        ssize_t bytes_read = read(pfd.fd, buffer, sizeof(buffer) - 1);
                        
                        if (bytes_read == 0) {
                            write(2, "[ERROR] Connexion fermée par le client\n", 41);
                            to_remove.push_back(pfd.fd);
                            continue;
                        }
                        else if (bytes_read < 0) {
                            write(2, "[ERROR] Erreur de lecture\n", 26);
                            to_remove.push_back(pfd.fd);
                            continue;
                        }
                        else if (bytes_read > 0) {
                            // Ajouter les nouvelles données à la requête existante
                            Request& request = handler_it->second.getRequest();
                            request.feed(buffer, bytes_read);

                            // Vérifier si la requête est maintenant complète
                            if (request.isComplete()) {
                                // Traiter la requête complète
                                handler_it->second.process();

                                // Envoyer la réponse
                                std::string response = handler_it->second.getResponse();
                                write(pfd.fd, response.c_str(), response.length());

                                // Marquer pour suppression
                                to_remove.push_back(pfd.fd);
                            }
                        }
                    }
                }
            }

            // On réinitialise revents comme auparavant
            pfd.revents = 0;
        }

        // Enlever définitivement les FDs marquées
        for (size_t i = 0; i < to_remove.size(); ++i) {
            removeFromPoll(to_remove[i]);
        }
    }
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
    // Nettoyer le handler avant de fermer le fd
    std::map<int, Handler>::iterator it = _handlers.find(fd);
    if (it != _handlers.end()) {
        _handlers.erase(it);
    }

    for (std::vector<struct pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
        if (it->fd == fd) {
            close(fd);
            char buf[64];
            snprintf(buf, sizeof(buf),
                     "[DEBUG] Fermeture & suppression FD %d du poll\n", fd);
            write(1, buf, strlen(buf));
            _pollfds.erase(it);
            return;
        }
    }
}
