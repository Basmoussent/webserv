# WebServ

A simple HTTP server implementation in C++.

## Project Roadmap

### Completed Features
- [x] Structure ServerConfig
- [x] Création des sockets, gestion multi-serveurs
- [x] Multiplexage avec poll()
- [x] Acceptation des connexions
- [x] Réception des requêtes (affichage brut)
- [x] Envoi de réponse basique

### To Do
- [ ] Parsing de la configuration
    - Lecture du fichier de config et parsing
    - Assignation des valeurs dans les structures
    - Gestion d'erreur de parsing

- [ ] Gestion des signaux
    - SIGINT + SIGTERM + SIGQUIT

- [ ] Parsing des requêtes HTTP
    - Création de la structure HttpRequest
    - Parsing de la méthode : GET/POST/DELETE/header/body
    - Parsing du path
    - Assignation des valeurs dans les structures

- [ ] Gestion des requêtes
    - Vérification des méthodes autorisées
    - Vérification des chemins
    - Gestion des redirections
    - Gestion des erreurs (404, 403, etc.)
    - Pages d'erreur personnalisées

- [ ] Gestion du contenu
    - Lecture des fichiers
    - Gestion des types de contenu
    - Gestion de l'index
    - Gestion de l'autoindex
    - Gestion des uploads

- [ ] Création des fichiers du serveur
    - Structure des dossiers
        - /www/ (racine du serveur)
            - /images/ (stockage des images)
            - /api/ (endpoints API)
            - /uploads/ (dossier pour les uploads)
            - /error_pages/ (pages d'erreur personnalisées)
    - Fichiers de base
        - index.html (page d'accueil)
        - style.css (styles)
        - script.js (fonctionnalités JavaScript)
    - Pages d'erreur
        - 404.html
        - 403.html
        - 500.html
    - Documentation API
        - api_doc.html
        - api_examples/

## How to Use

1. Compile the project:
```bash
make
```

2. Run the server (pour l'instant pas de config.file dans les params parce que c déjà hard codé):
```bash
./webserv
```

3. Test with curl:
```bash
curl http://127.0.0.1:8080
curl http://127.0.0.1:9090
```

## Server File Structure
```
www/
├── images/
│   ├── logo.png
│   └── background.jpg
├── api/
│   ├── users/
│   └── data/
├── uploads/
├── error_pages/
│   ├── 404.html
│   ├── 403.html
│   └── 500.html
├── index.html
├── style.css
└── script.js
``` 