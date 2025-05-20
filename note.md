# Plan d’implémentation complet de Webserv

## 1. Initialisation

1. **Parser la configuration**  
   - Charger `default.conf` (ou le fichier passé en argument)  
   - Extraire :  
     - Tous les couples **IP:port** (`listen`) → nombre de sockets d’écoute  
     - Directives globales (`server_name`, `root`, `index`, `error_page`, `client_max_body_size`)  
     - Blocs `location { … }` (méthodes autorisées, `autoindex`, `upload_dir`, `cgi_pass`)

2. **Créer / configurer chaque socket d’écoute**  
```cpp
int sock = socket(AF_INET, SOCK_STREAM, 0);
if (sock < 0) { perror("socket"); exit(1); }
int yes = 1;
setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
struct sockaddr_in addr = {0};
addr.sin_family = AF_INET;
addr.sin_port   = htons(port);
addr.sin_addr.s_addr = INADDR_ANY;
bind(sock, (struct sockaddr*)&addr, sizeof(addr));
listen(sock, SOMAXCONN);
fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
```

3. **Installer les handlers de signaux**  
```cpp
signal(SIGINT,  handle_exit);
signal(SIGTERM, handle_exit);
```

## 2. Boucle d’événements

1. **Choix du multiplexeur**  
   - `poll()` (portable), ou  
   - `epoll` sur Linux, ou  
   - `kqueue` sur macOS/BSD

2. **Structure générale**  
```cpp
// fds = listen_sockets + client_sockets
while (running) {
  int n = wait_for_events(fds);  // poll/epoll_wait/kevent
  for (auto& ev : active_events) {
    int fd = ev.fd;
    if (fd est listen_socket) {
      // accept
      int client = accept(fd, …);
      fcntl(client, F_SETFL, O_NONBLOCK);
      add_fd_to_monitor(client);
    }
    else if (ev.readable) {
      on_readable(fd);
    }
    else if (ev.writable) {
      on_writable(fd);
    }
  }
}
```

3. **Lecture et parsing partiel**  
```cpp
void on_readable(int fd) {
  char buf[4096];
  int n = recv(fd, buf, sizeof(buf), 0);
  if (n <= 0) { close_connection(fd); return; }
  conn[fd].in.append(buf, n);
  if (! conn[fd].headers_parsed
      && conn[fd].in.find("\r\n\r\n") != npos) {
    parse_headers(fd);
  }
  if (conn[fd].headers_parsed
      && conn[fd].in.size() >= conn[fd].expected_body) {
    build_request_and_dispatch(fd);
  }
}
```

4. **Exemple de parsing de la request-line**  
```cpp
std::string block = conn[fd].in.substr(0, pos);
std::istringstream ss(block);
std::string request_line;
std::getline(ss, request_line);               // "GET /foo HTTP/1.1"
std::istringstream rl(request_line);
rl >> method >> uri >> version;
// puis loop pour chaque header: "Name: value"
```

## 3. Dispatch & génération de la réponse

1. **Choisir le handler**  
```cpp
if (method == "GET")       handler = staticHandler;
else if (method == "POST") handler = cgiOrUploadHandler;
else if (method == "DELETE") handler = deleteHandler;
else                       handler = errorHandler(405);
```

2. **Générer la réponse**  
```cpp
Response res = handler->handle(request, serverConfig);
conn[fd].out = res.toString();  // status line + headers + "\r\n" + body
```

## 4. Écriture non-bloquante

```cpp
void on_writable(int fd) {
  size_t &sent = conn[fd].bytes_sent;
  const std::string &out = conn[fd].out;
  while (sent < out.size()) {
    int n = send(fd, out.data() + sent, out.size() - sent, 0);
    if (n > 0) sent += n;
    else if (n < 0 && errno == EAGAIN) return;
    else { close_connection(fd); return; }
  }
  // réponse complète envoyée
  post_send_cleanup(fd);
}
```

## 5. Gestion de la connexion

- **Fermer** et retirer du multiplexeur si :  
  - `Connection: close`  
  - réponse envoyée ET pas de keep-alive  
- **Sinon** :  
  - Réarmer en **POLLIN** pour la requête suivante (keep-alive / pipelining)

```cpp
void post_send_cleanup(int fd) {
  if (conn[fd].close_after_send) {
    close_connection(fd);
  } else {
    modify_fd_event(fd, POLLIN);
    conn[fd].reset_for_next_request();
  }
}
```

## 6. Logging

- **`access.log`** (après réponse) :  
  ```
  [YYYY-MM-DD HH:MM:SS] "METHOD URI HTTP/1.1" STATUS SIZE DURATION_ms
  ```
- **`error.log`** (à l’erreur) :  
  ```cpp
  fprintf(err_fp, "[%s] fd=%d %s
",
          timestamp(), fd, strerror(errno));
  ```

## 7. Nettoyage et terminaison

- **Handler de signaux** (`handle_exit`) :  
```cpp
for (int fd : listen_sockets) close(fd);
for (auto& p : conn)     close(p.first);
exit(0);
```
- Fermer tous les logs et libérer les ressources.