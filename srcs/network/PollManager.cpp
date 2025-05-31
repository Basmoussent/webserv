#include "../../includes/PollManager.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <algorithm>


// Helper pour mettre un fd en non-bloquant
static void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

PollManager::PollManager(SocketHandler& socketHandler)
    : _socketHandler(socketHandler) {}

PollManager::~PollManager() {
    for (size_t i = 0; i < _pollfds.size(); ++i) {
        close(_pollfds[i].fd);
    }
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
                        // On ne teste pas errno : on ne fait que logger et on continue
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

                    char buffer[1024];
                    int bytes = recv(pfd.fd, buffer, sizeof(buffer) - 1, 0);
                    if (bytes <= 0) {
                        // bytes == 0 => fermeture propre du client,
                        // bytes < 0 => on considère que c'est fini aussi.
                        write(1, "[DEBUG] Fermeture du client détectée\n", 38);
                        to_remove.push_back(pfd.fd);
                        continue;
                    }

                    // On a bien reçu des bytes > 0 :
                    buffer[bytes] = '\0';
                    write(1, "[DEBUG] Requête client reçue:\n", 31);
                    write(1, buffer, bytes);

                    // Générer une réponse fixe (Hello, World) et l'envoyer
                    const char* response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 13\r\n"
                        "\r\n"
                        "Hello, World!";
                    send(pfd.fd, response, strlen(response), 0);

                    // On ferme et supprime immédiatement cette socket client
                    to_remove.push_back(pfd.fd);
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
