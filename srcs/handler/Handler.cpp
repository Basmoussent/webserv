#include "Webserv.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <string>
#include <cstdlib>
#include <map>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>


std::string Handler::intToString(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

std::string Handler::sizeToString(size_t value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

Handler::Handler(Request& request, ConfigParser& configParser)
    : _request(request), _configParser(configParser), _statusCode(200), _isValid(true)
{
}

Handler::~Handler()
{
    clear();
}

Request& Handler::getRequest()
{
	return _request;
}

void Handler::process()
{
    std::cout << "[DEBUG] Début du traitement de la requête" << std::endl;
    std::cout << "[DEBUG] Méthode: " << _request.getMethod() << std::endl;
    std::cout << "[DEBUG] URI: " << _request.getUri() << std::endl;
    
	if (_request.isValid())
	{
        size_t i = 0;
        Server server;

        if (_configParser.getServers().empty() || _configParser.getServers()[0].locations.empty())
        {
            std::cout << "[DEBUG] Aucun serveur ou location configuré" << std::endl;
            setStatusCode(500);
            _response = buildResponse(500, "Internal Server Error", "text/plain");
            return;
        }
        for (i = 0; i < _configParser.getServers().size(); i++)
        {
            server = _configParser.getServers()[i];
            if (_request.getHeader("Host") == server.instruct["host"] + ":" + server.instruct["listen"] || _request.getHeader("Host") == server.instruct["server_name"])
                break;
        }
        if (i == _configParser.getServers().size())
        {
            setStatusCode(404);
            _response = buildResponse(404, "Not Found", "text/plain");
            return;
        }

        std::string contentLength = _request.getHeader("Content-Length");
        if (!contentLength.empty()) {
            size_t bodySize = std::atoi(contentLength.c_str());
            size_t maxSize = 0;
            bool locationFound = false;
            size_t j = 0;
            for (j = 0; j < server.locations.size(); j++) {
                if (server.locations[j].path == _request.getUri()) {
                    locationFound = true;
                    if (!server.locations[j].instruct["client_max_body_size"].empty()) {
                        maxSize = std::atoi(server.locations[j].instruct["client_max_body_size"].c_str());
                    }
                    break;
                }
            }
            
            if (!locationFound || maxSize == 0) {
                if (!server.instruct["client_max_body_size"].empty()) {
                    maxSize = std::atoi(server.instruct["client_max_body_size"].c_str());
                }
            }
            
            if (maxSize > 0 && bodySize > maxSize) {
                setStatusCode(413);
                _response = buildResponse(413, "Request Entity Too Large", "text/plain");
                return;
            }
        }

        bool locationFound = false;
        size_t j = 0;
        for (j = 0; j < server.locations.size(); j++)
        {
            if (server.locations[j].path == _request.getUri())
            {
                std::string allowedMethods = server.locations[j].instruct["allow_methods"];
                if (allowedMethods.find(_request.getMethod()) == std::string::npos)
                {
                    if (_request.getMethod() != "GET" &&  _request.getMethod() != "POST" && _request.getMethod() != "DELETE" && _request.getMethod() != "HEAD")
                    {
                        setStatusCode(501);
                        _response = buildResponse(501, "Not Implemented", "text/plain");
                        return;
                    }
                    setStatusCode(405);
                    _response = buildResponse(405, "Method Not Allowed", "text/plain");
                    return;
                }
                locationFound = true;
                break;
            }
        }
        if (!locationFound) {
            for (j = 0; j < server.locations.size(); j++) {
                if (server.locations[j].path == "/") {
                    locationFound = true;
                    break;
                }
            }
        }
        if (!locationFound) {
            setStatusCode(404);
            _response = buildResponse(404, "Not Found", "text/plain");
            return;
        }

        if (_request.getUri().find("/cgi-bin/") == 0 || (_request.getUri().find("/cgi-bin") == 0 && _request.getUri().length() == 8))
            handleCGI();
        else if (_request.getMethod() == "GET")
			handleGet(server, j);
		else if (_request.getMethod() == "POST")
			handlePost(server.locations[j].path);
		else if (_request.getMethod() == "DELETE")
			handleDelete();
        else if (_request.getMethod() == "HEAD")
            handleHead(server, j);
		setValid(true);
	}
	else
	{
		setStatusCode(400);
		_response = buildResponse(400, "Bad Request", "text/plain");
        std::cout << "Invalid request: " << _request << std::endl;
	}
}

void Handler::handleCGI()
{
    Server server;
    Location location;
    bool found = false;
    
    for (size_t i = 0; i < _configParser.getServers().size(); i++) {
        server = _configParser.getServers()[i];
        if (_request.getHeader("Host") == server.instruct["host"] + ":" + server.instruct["listen"] || 
            _request.getHeader("Host") == server.instruct["server_name"]) {
            for (size_t j = 0; j < server.locations.size(); j++) {
                if (server.locations[j].path == "/cgi-bin") {
                    location = server.locations[j];
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }

    if (!found) {
        setStatusCode(404);
        _response = buildResponse(404, "Not Found", "text/plain");
        return;
    }

    std::string cgiPath = location.instruct["cgi_path"];
    std::string cgiExt = location.instruct["cgi_ext"];
    std::string index = location.instruct["index"];
    std::string root = location.instruct["root"];

    std::vector<std::string> extensions;
    std::istringstream extStream(cgiExt);
    std::string ext;
    while (std::getline(extStream, ext, ' ')) {
        if (!ext.empty()) {
            extensions.push_back(ext);
        }
    }

    std::vector<std::string> interpreters;
    std::istringstream pathStream(cgiPath);
    std::string interpreter;
    while (std::getline(pathStream, interpreter, ' ')) {
        if (!interpreter.empty()) {
            interpreters.push_back(interpreter);
        }
    }

    std::string basePath = root.empty() ? "./cgi-bin" : root;
    std::string filename = basePath + "/" + _request.getUri().substr(_request.getUri().find_last_of("/") + 1);
    std::cout << "Filename: " << filename << std::endl;
    struct stat fileStat;

    if (stat(filename.c_str(), &fileStat) != 0 && !index.empty()) {
        filename = basePath + "/" + index;
        if (stat(filename.c_str(), &fileStat) != 0) {
            setStatusCode(404);
            _response = buildResponse(404, "Not Found", "text/plain");
            return;
        }
    } else if (stat(filename.c_str(), &fileStat) != 0) {
        setStatusCode(404);
        _response = buildResponse(404, "Not Found", "text/plain");
        return;
    }

    std::string extension = filename.substr(filename.find_last_of("."));
    bool validExtension = false;
    for (std::vector<std::string>::iterator it = extensions.begin(); it != extensions.end(); ++it) {
        if (extension == *it) {
            validExtension = true;
            break;
        }
    }

    if (!validExtension) {
        setStatusCode(403);
        _response = buildResponse(403, "Forbidden", "text/plain");
        return;
    }

    if (_request.getMethod() == "POST") {
        std::ifstream file(filename.c_str(), std::ios::binary);
        if (!file) {
            setStatusCode(500);
            _response = buildResponse(500, "Internal Server Error", "text/plain");
            return;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();
        setStatusCode(200);
        _response = buildResponse(200, content, "text/plain");
        return;
    }
    else if (_request.getMethod() == "DELETE") {
        if (remove(filename.c_str()) == 0) {
            setStatusCode(200);
            _response = buildResponse(200, "Script deleted successfully", "text/plain");
        } else {
            setStatusCode(500);
            _response = buildResponse(500, "Failed to delete script", "text/plain");
        }
        return;
    }
    else if (_request.getMethod() == "GET") {
        std::map<std::string, std::string> env;
        env["REQUEST_METHOD"] = _request.getMethod();
        env["QUERY_STRING"] = _request.getQueryString();
        env["CONTENT_LENGTH"] = _request.getHeader("Content-Length");
        env["CONTENT_TYPE"] = _request.getHeader("Content-Type");
        env["SCRIPT_NAME"] = _request.getUri();
        env["SERVER_PROTOCOL"] = "HTTP/1.1";
        env["SERVER_SOFTWARE"] = "webserv/1.0";
        env["REMOTE_ADDR"] = server.instruct["host"] + ":" + server.instruct["listen"];
        env["REMOTE_PORT"] = server.instruct["listen"];

        std::string command;
        if (extension == ".py" && interpreters.size() > 0) {
            command = interpreters[0] + " " + filename;
        } else if (extension == ".sh" && interpreters.size() > 1) {
            command = interpreters[1] + " " + filename;
        } else {
            setStatusCode(500);
            _response = buildResponse(500, "Internal Server Error", "text/plain");
            return;
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            setStatusCode(500);
            _response = buildResponse(500, "Internal Server Error", "text/plain");
            return;
        }

        pid_t pid = fork();
        if (pid == -1) {
            close(pipefd[0]);
            close(pipefd[1]);
            setStatusCode(500);
            _response = buildResponse(500, "Internal Server Error", "text/plain");
            return;
        }

        if (pid == 0) {
            std::vector<std::string> envStrings;
            for (std::map<std::string, std::string>::iterator it = env.begin(); it != env.end(); ++it) {
                envStrings.push_back(it->first + "=" + it->second);
            }
            
            std::vector<char*> envArray;
            for (std::vector<std::string>::iterator it = envStrings.begin(); it != envStrings.end(); ++it) {
                envArray.push_back(const_cast<char*>(it->c_str()));
            }
            envArray.push_back(NULL);

            std::vector<std::string> args;
            args.push_back("/bin/sh");
            args.push_back("-c");
            args.push_back(command);

            std::vector<char*> argArray;
            for (std::vector<std::string>::iterator it = args.begin(); it != args.end(); ++it) {
                argArray.push_back(const_cast<char*>(it->c_str()));
            }
            argArray.push_back(NULL);

            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            execve("/bin/sh", &argArray[0], &envArray[0]);
            exit(1);
        }

        close(pipefd[1]);
        
        std::string output;
        char readBuffer[4096];
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[0], readBuffer, sizeof(readBuffer) - 1)) > 0) {
            readBuffer[bytes_read] = '\0';
            output += readBuffer;
        }
        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            setStatusCode(200);
            std::ostringstream len;
            len << output.size();
            _response = buildResponse(200, output, "text/plain");
        } else {
            setStatusCode(500);
            _response = buildResponse(500, "Internal Server Error", "text/plain");
        }
    }
    else {
        setStatusCode(405);
        _response = buildResponse(405, "Method Not Allowed", "text/plain");
    }
}

std::string Handler::getMimeType(const std::string& path) const {
    std::string extension = path.substr(path.find_last_of(".") + 1);
    std::map<std::string, std::string> mimeTypes;
    
    mimeTypes["html"] = "text/html";
    mimeTypes["htm"] = "text/html";
    mimeTypes["css"] = "text/css";
    mimeTypes["js"] = "application/javascript";
    mimeTypes["json"] = "application/json";
    mimeTypes["png"] = "image/png";
    mimeTypes["jpg"] = "image/jpeg";
    mimeTypes["jpeg"] = "image/jpeg";
    mimeTypes["gif"] = "image/gif";
    mimeTypes["ico"] = "image/x-icon";
    mimeTypes["txt"] = "text/plain";
    mimeTypes["pdf"] = "application/pdf";
    mimeTypes["xml"] = "application/xml";
    mimeTypes["zip"] = "application/zip";
    
    std::map<std::string, std::string>::iterator it = mimeTypes.find(extension);
    if (it != mimeTypes.end()) {
        return it->second;
    }
    return "application/octet-stream";
}

std::string Handler::buildResponse(int statusCode, const std::string& content, const std::string& contentType) const {
    std::string statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 201: statusText = "Created"; break;
        case 204: statusText = "No Content"; break;
        case 301: statusText = "Moved Permanently"; break;
        case 400: statusText = "Bad Request"; break;
        case 401: statusText = "Unauthorized"; break;
        case 403: statusText = "Forbidden"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        case 413: statusText = "Request Entity Too Large"; break;
        case 500: statusText = "Internal Server Error"; break;
        case 501: statusText = "Not Implemented"; break;
        default: statusText = "Unknown";
    }
    
    std::string response;
    response.reserve(512 + content.length()); // Pré-allouer de l'espace pour éviter les réallocations
    
    response = "HTTP/1.1 " + intToString(statusCode) + " " + statusText + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + sizeToString(content.length()) + "\r\n";
    if (statusCode == 405) 
        response += "Allow: GET, POST, DELETE\r\n";
    response += "Server: webserv/1.0\r\n";
    response += "Connection: Keep-Alive\r\n";
    response += "Accept-Ranges: bytes\r\n";
    response += "Date: " + getCurrentDate() + "\r\n";
    response += "\r\n";
    response += content;
    
    return response;
}

std::string Handler::getCurrentDate() const {
    char buffer[100];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &tm);
    return std::string(buffer);
}

std::string Handler::normalizePath(const std::string& path) {
    if (path.find("..") != std::string::npos || 
        path.find("//") != std::string::npos ||
        path.find("\\") != std::string::npos ||
        path.find("%2e") != std::string::npos ||
        path.find("%2f") != std::string::npos ||
        path.find("%5c") != std::string::npos) {
        setStatusCode(403);
        _response = buildResponse(403, "Forbidden - Path traversal attempt detected", "text/plain");
        return "";
    }

    std::vector<std::string> parts;
    std::string current;
    std::istringstream iss(path);
    
    while (std::getline(iss, current, '/')) {
        if (current.empty() || current == ".") continue;
        if (current == "..") {
            if (!parts.empty()) parts.pop_back();
        } else {
            parts.push_back(current);
        }
    }
    
    std::string normalized = "/";
    for (size_t i = 0; i < parts.size(); ++i) {
        normalized += parts[i];
        if (i < parts.size() - 1) normalized += "/";
    }
    return normalized;
}

void Handler::handleGet(Server serv, int j)
{
    struct stat buffer;
    std::string root;
    std::string uri = _request.getUri();
    
    if (uri.find("..") != std::string::npos) {
        std::string normalized = normalizePath(uri);
        std::cout << "Normalized URI: " << normalized << std::endl;
        if (normalized.empty())
            return;
        if (normalized != uri) {
            setStatusCode(301);
            std::string redirectUrl = normalized;
            if (normalized[normalized.length() - 1] != '/') {
                redirectUrl += "/";
            }
            std::string response = "HTTP/1.1 301 Moved Permanently\r\n";
            response += "Location: " + redirectUrl + "\r\n";
            response += "\r\n";
            _response = response;
            return;
        }
    }
    
    if (!serv.instruct["root"].empty())
        root = serv.instruct["root"];
    else if (!serv.locations[j].instruct["root"].empty())
        root = serv.locations[j].instruct["root"];
    else {
        setStatusCode(500);
        _response = buildResponse(500, "Internal Server Error", "text/plain");
        return;
    }
    if (root.substr(0, 2) == "./") {
        root = root.substr(2);
    }
    std::vector<std::string> indexFiles;
    if (!serv.locations[j].instruct["index"].empty()) {
        std::istringstream iss(serv.locations[j].instruct["index"]);
        std::string indexFile;
        while (std::getline(iss, indexFile, ' ')) {
            if (!indexFile.empty()) {
                indexFiles.push_back(indexFile);
            }
        }
    } else if (!serv.instruct["index"].empty()) {
        std::istringstream iss(serv.instruct["index"]);
        std::string indexFile;
        while (std::getline(iss, indexFile, ' ')) {
            if (!indexFile.empty()) {
                indexFiles.push_back(indexFile);
            }
        }
    } else {
        indexFiles.push_back("index.html");
    }
    std::string fullPath = root + (uri[0] == '/' ? uri : "/" + uri);
    if (stat(fullPath.c_str(), &buffer) != 0) {
        setStatusCode(404);
        std::string errorPage = getErrorPage(404);
        _response = buildResponse(404, errorPage, "text/html");
        return;
    }
    if (S_ISDIR(buffer.st_mode)) {
        bool indexFound = false;
        for (std::vector<std::string>::iterator it = indexFiles.begin(); it != indexFiles.end(); ++it) {
            std::string indexPath = fullPath + "/" + *it;
            if (stat(indexPath.c_str(), &buffer) == 0) {
                fullPath = indexPath;
                indexFound = true;
                break;
            }
        }
        if (!indexFound) {
            if (serv.locations[j].instruct["autoindex"] == "on") {
                std::string autoindexHtml = generateDirectoryListing(fullPath, uri);
                setStatusCode(200);
                _response = buildResponse(200, autoindexHtml, "text/html");
                return;
            } else {
                setStatusCode(403);
                std::string errorPage = getErrorPage(403);
                _response = buildResponse(403, errorPage, "text/html");
                return;
            }
        }
    }
    
    std::ifstream file(fullPath.c_str(), std::ios::binary);
    if (!file) {
        setStatusCode(500);
        _response = buildResponse(500, "Internal Server Error", "text/plain");
        return;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    std::string contentType = getMimeType(fullPath);
    setStatusCode(200);
    _response = buildResponse(200, content, contentType);
}

std::string Handler::generateDirectoryListing(const std::string& path, const std::string& uri) const {
    std::string html = "<!DOCTYPE html>\n<html>\n<head>\n";
    html += "<title>Index of " + uri + "</title>\n";
    html += "<style>\n";
    html += "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    html += "h1 { color: #333; }\n";
    html += "table { border-collapse: collapse; width: 100%; }\n";
    html += "th, td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }\n";
    html += "tr:hover { background-color: #f5f5f5; }\n";
    html += "a { text-decoration: none; color: #0066cc; }\n";
    html += "a:hover { text-decoration: underline; }\n";
    html += "</style>\n</head>\n<body>\n";
    html += "<h1>Index of " + uri + "</h1>\n";
    html += "<table>\n<tr><th>Name</th><th>Last modified</th><th>Size</th></tr>\n";

    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            
            std::string fullPath = path + "/" + entry->d_name;
            struct stat st;
            if (stat(fullPath.c_str(), &st) != 0) continue;

            std::string name = entry->d_name;
            if (S_ISDIR(st.st_mode)) name += "/";

            char date[100];
            strftime(date, sizeof(date), "%d-%b-%Y %H:%M", localtime(&st.st_mtime));
            
            std::string size = S_ISDIR(st.st_mode) ? "-" : sizeToString(st.st_size);
            
            html += "<tr><td><a href=\"" + name + "\">" + name + "</a></td>";
            html += "<td>" + std::string(date) + "</td>";
            html += "<td>" + size + "</td></tr>\n";
        }
        closedir(dir);
    }

    html += "</table>\n</body>\n</html>";
    return html;
}

std::string Handler::getErrorPage(int statusCode) const {
    std::string errorPage = "<!DOCTYPE html>\n<html>\n<head>\n";
    errorPage += "<title>Error " + intToString(statusCode) + "</title>\n";
    errorPage += "<style>\n";
    errorPage += "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\n";
    errorPage += "h1 { color: #333; }\n";
    errorPage += "p { color: #666; }\n";
    errorPage += "</style>\n</head>\n<body>\n";
    errorPage += "<h1>Error " + intToString(statusCode) + "</h1>\n";
    
    switch (statusCode) {
        case 403:
            errorPage += "<p>Access Forbidden</p>\n";
            break;
        case 404:
            errorPage += "<p>Page Not Found</p>\n";
            break;
        case 500:
            errorPage += "<p>Internal Server Error</p>\n";
            break;
        default:
            errorPage += "<p>An error occurred</p>\n";
    }
    
    errorPage += "</body>\n</html>";
    return errorPage;
}

std::string Handler::getUploadDirectory(const Server& server, const Location& location) const {
    std::map<std::string, std::string>::const_iterator it = location.instruct.find("upload_dir");
    if (it != location.instruct.end()) {
        return it->second;
    }
    
    it = server.instruct.find("upload_dir");
    if (it != server.instruct.end()) {
        return it->second;
    }
    
    return "./html/uploads_default";
}

bool Handler::handleFileUpload(const std::string& uploadDir, const std::string& filename, const std::string& content) {
    struct stat st;
    
    // Vérifier si le répertoire existe et a les bonnes permissions
    if (stat(uploadDir.c_str(), &st) != 0) {
        if (mkdir(uploadDir.c_str(), 0755) != 0) {
            std::cerr << "Error: Failed to create upload directory: " << uploadDir << " (errno: " << errno << ")" << std::endl;
            return false;
        }
    } else if (!(st.st_mode & S_IWUSR)) {
        std::cerr << "Error: Upload directory is not writable: " << uploadDir << std::endl;
        return false;
    }

    // Vérifier si le nom de fichier est valide
    if (filename.empty() || filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos) {
        std::cerr << "Error: Invalid filename: " << filename << std::endl;
        return false;
    }

    std::string baseFilename = filename;
    std::string extension = "";
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        baseFilename = filename.substr(0, dotPos);
        extension = filename.substr(dotPos);
    }

    std::string filepath = uploadDir + "/" + filename;
    int counter = 1;
    
    // Vérifier si le fichier existe déjà et générer un nouveau nom si nécessaire
    while (stat(filepath.c_str(), &st) == 0) {
        filepath = uploadDir + "/" + baseFilename + "_" + intToString(counter) + extension;
        counter++;
        if (counter > 1000) { // Limite de sécurité
            std::cerr << "Error: Too many files with similar names" << std::endl;
            return false;
        }
    }
    
    std::ofstream file(filepath.c_str(), std::ios::out | std::ios::binary);
    if (!file) {
        std::cerr << "Error: Failed to create file: " << filepath << " (errno: " << errno << ")" << std::endl;
        return false;
    }
    
    try {
        file.write(content.c_str(), content.size());
        if (file.fail()) {
            std::cerr << "Error: Failed to write to file: " << filepath << " (errno: " << errno << ")" << std::endl;
            file.close();
            remove(filepath.c_str()); // Nettoyer en cas d'échec
            return false;
        }
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error: Exception while writing file: " << e.what() << std::endl;
        file.close();
        remove(filepath.c_str()); // Nettoyer en cas d'échec
        return false;
    }
}

std::string Handler::extractFilenameFromMultipart(const std::string& body, const std::string& boundary) const {
    std::string filename;
    size_t start = body.find("--" + boundary);
    if (start != std::string::npos) {
        size_t pos = body.find("filename=\"", start);
        if (pos != std::string::npos) {
            pos += 10;
            size_t end = body.find("\"", pos);
            if (end != std::string::npos) {
                filename = body.substr(pos, end - pos);
            }
        }
    }
    return filename;
}

std::string Handler::extractContentFromMultipart(const std::string& body, const std::string& boundary) const {
    std::string content;
    
    size_t start = body.find("\r\n\r\n");
    if (start != std::string::npos) {
        start += 4;
        
        size_t end = body.find("--" + boundary, start);
        if (end != std::string::npos) {
            content = body.substr(start, end - start);
            
            while (!content.empty() && (content[content.length() - 1] == '\r' || content[content.length() - 1] == '\n')) {
                content.erase(content.length() - 1);
            }
        }
    }
    return content;
}

void Handler::handlePost(std::string& path)
{
    const std::string& body = _request.getBody();
    std::string pathCheck = path;
    if (_request.getUri() != path && _request.getUri() != (pathCheck + "/"))
    {
        setStatusCode(500);
        _response = buildResponse(500, "Internal Server Error - Invalid path", "text/plain");
        return;
    }

    Server server;
    Location location;
    bool found = false;
    
    for (size_t i = 0; i < _configParser.getServers().size(); i++) {
        server = _configParser.getServers()[i];
        if (_request.getHeader("Host") == server.instruct["host"] + ":" + server.instruct["listen"]  || 
            _request.getHeader("Host") == server.instruct["server_name"]) {
            for (size_t j = 0; j < server.locations.size(); j++) {
                if (server.locations[j].path == path) {
                    location = server.locations[j];
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }

    if (!found) {
        setStatusCode(404);
        _response = buildResponse(404, "Not Found - No matching location found", "text/plain");
        return;
    }

    if (_request.getHeader("Content-Type").find("multipart/form-data") != std::string::npos) {
        std::string contentType = _request.getHeader("Content-Type");
        std::cout << "Content-Type: " << contentType << std::endl;
        
        size_t boundaryPos = contentType.find("boundary=");
        if (boundaryPos == std::string::npos) {
            setStatusCode(400);
            _response = buildResponse(400, "Bad Request - No boundary found in Content-Type", "text/plain");
            return;
        }
        
        std::string boundary = contentType.substr(boundaryPos + 9);
        if (boundary.empty()) {
            setStatusCode(400);
            _response = buildResponse(400, "Bad Request - Empty boundary", "text/plain");
            return;
        }

        std::string filename = extractFilenameFromMultipart(body, boundary);
        if (filename.empty()) {
            setStatusCode(400);
            _response = buildResponse(400, "Bad Request - No filename found in request", "text/plain");
            return;
        }

        std::string content = extractContentFromMultipart(body, boundary);
        if (content.empty()) {
            setStatusCode(400);
            _response = buildResponse(400, "Bad Request - No content found in request", "text/plain");
            return;
        }

        std::string uploadDir = getUploadDirectory(server, location);
        if (uploadDir.empty()) {
            setStatusCode(500);
            _response = buildResponse(500, "Internal Server Error - No upload directory configured", "text/plain");
            return;
        }

        if (handleFileUpload(uploadDir, filename, content)) {
            setStatusCode(201);
            std::string location = "Location: " + uploadDir + "/" + filename;
            _response = buildResponse(201, location, "text/plain");
        } else {
            setStatusCode(500);
            _response = buildResponse(500, "Internal Server Error - Failed to save uploaded file", "text/plain");
        }
        return;
    }

    std::string root;
    if (!location.instruct["root"].empty()) {
        root = location.instruct["root"];
    } else if (!server.instruct["root"].empty()) {
        root = server.instruct["root"];
    } else {
        root = "./html";
    }

    if (root.substr(0, 2) == "./") {
        root = root.substr(2);
    }

    std::string logBaseDir = root + "/log";
    struct stat st;
    if (stat(logBaseDir.c_str(), &st) != 0) {
        if (mkdir(logBaseDir.c_str(), 0755) != 0) {
            setStatusCode(500);
            _response = buildResponse(500, "Internal Server Error - Could not create log directory", "text/plain");
            return;
        }
    }

    std::string logPath = logBaseDir + path;
    if (stat(logPath.c_str(), &st) != 0) {
        if (mkdir(logPath.c_str(), 0755) != 0) {
            setStatusCode(500);
            _response = buildResponse(500, "Internal Server Error - Could not create log subdirectory", "text/plain");
            return;
        }
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    std::stringstream filename;
    filename << logPath << "/post_"
             << (t->tm_year + 1900)
             << std::setw(2) << std::setfill('0') << t->tm_mon + 1
             << std::setw(2) << std::setfill('0') << t->tm_mday
             << "_"
             << std::setw(2) << std::setfill('0') << t->tm_hour
             << std::setw(2) << std::setfill('0') << t->tm_min
             << std::setw(2) << std::setfill('0') << t->tm_sec
             << ".log";

    std::string filepath = filename.str();
    std::cout << "Log file path: " << filepath << std::endl;
    
    std::ofstream file(filepath.c_str(), std::ios::out | std::ios::trunc);
    std::string tt = filepath.substr(filepath.length() - 25, filepath.length() - 1);
    if (file) {
        file << "=== POST Request Log ===\n";
        file << "Timestamp: " <<  tt << "\n";
        file << "Path: " << path << "\n";
        file << "Content-Type: " << _request.getHeader("Content-Type") << "\n";
        file << "Content-Length: " << _request.getHeader("Content-Length") << "\n";
        file << "=== Body ===\n";
        file << body << "\n";
        file << "=== End of Log ===\n";
        file.close();
        
        setStatusCode(201);
        std::string location = "Location: " + filepath;
        _response = buildResponse(201, location, "text/plain");
    } else {
        setStatusCode(500);
        _response = buildResponse(500, "Internal Server Error - Could not write to log file", "text/plain");
    }
}

void Handler::handleDelete()
{
    std::string uri = _request.getUri();
    if (uri.empty() || uri == "/")
    {
        setStatusCode(400);
        _response = buildResponse(400, "Missing filename in URI", "text/plain");
        return;
    }

    Server server;
    Location location;
    bool found = false;
    
    for (size_t i = 0; i < _configParser.getServers().size(); i++) {
        server = _configParser.getServers()[i];
        if (_request.getHeader("Host") == server.instruct["host"] + ":" + server.instruct["listen"] || 
            _request.getHeader("Host") == server.instruct["server_name"]) {
            for (size_t j = 0; j < server.locations.size(); j++) {
                if (server.locations[j].path == "/" || uri.find(server.locations[j].path) == 0) {
                    location = server.locations[j];
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }

    if (!found) {
        setStatusCode(404);
        _response = buildResponse(404, "Not Found", "text/plain");
        return;
    }

    std::string root;
    if (!location.instruct["root"].empty()) {
        root = location.instruct["root"];
    } else if (!server.instruct["root"].empty()) {
        root = server.instruct["root"];
    } else {
        root = "./html";
    }

    if (root.substr(0, 2) == "./") {
        root = root.substr(2);
    }

    std::string fullPath = root + uri;
    if (remove(fullPath.c_str()) == 0)
    {
        setStatusCode(200);
        std::string message = "File deleted successfully: " + uri;
        _response = buildResponse(200, message, "text/plain");
    }
    else
    {
        setStatusCode(404);
        std::string message = "File not found: " + uri;
        _response = buildResponse(404, message, "text/plain");
    }
}

void Handler::handleHead(Server serv, int j) {
    std::string root;
    if (!serv.locations[j].instruct["root"].empty()) {
        root = serv.locations[j].instruct["root"];
    } else if (!serv.instruct["root"].empty()) {
        root = serv.instruct["root"];
    } else {
        root = "./html";
    }

    if (root.substr(0, 2) == "./") {
        root = root.substr(2);
    }

    std::string path = root + _request.getUri();
    struct stat buffer;
    
    if (stat(path.c_str(), &buffer) == 0) {
        if (S_ISDIR(buffer.st_mode)) {
            std::string autoindex = serv.locations[j].instruct["autoindex"];
            if (autoindex == "on") {
                setStatusCode(200);
                _response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
                return;
            } else {
                std::vector<std::string> indexFiles;
                if (!serv.locations[j].instruct["index"].empty()) {
                    std::istringstream iss(serv.locations[j].instruct["index"]);
                    std::string indexFile;
                    while (std::getline(iss, indexFile, ' ')) {
                        if (!indexFile.empty()) {
                            indexFiles.push_back(indexFile);
                        }
                    }
                } else if (!serv.instruct["index"].empty()) {
                    std::istringstream iss(serv.instruct["index"]);
                    std::string indexFile;
                    while (std::getline(iss, indexFile, ' ')) {
                        if (!indexFile.empty()) {
                            indexFiles.push_back(indexFile);
                        }
                    }
                } else {
                    indexFiles.push_back("index.html");
                }

                for (std::vector<std::string>::iterator it = indexFiles.begin(); it != indexFiles.end(); ++it) {
                    std::string indexPath = path + "/" + *it;
                    if (stat(indexPath.c_str(), &buffer) == 0) {
                        setStatusCode(200);
                        _response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
                        return;
                    }
                }
                setStatusCode(403);
                _response = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";
                return;
            }
        } else {
            setStatusCode(200);
            std::string contentType = getMimeType(path);
            _response = "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "\r\nContent-Length: 0\r\n\r\n";
            return;
        }
    }
    
    setStatusCode(404);
    _response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";
}

int Handler::getStatusCode() const
{
	return _statusCode;
}

const std::string& Handler::getResponse() const
{
	return _response;
}

std::string Handler::getResponse()
{
	return _response;
}

bool Handler::isValid() const
{
	return _isValid;
}

void Handler::setStatusCode(int statusCode)
{
	_statusCode = statusCode;
}

void Handler::setResponse(const std::string& response)
{
	_response = response;
}

void Handler::setValid(bool isValid)
{
    _isValid = isValid;
}

void Handler::clear()
{
    _response.clear();
    _statusCode = 200;
    _isValid = true;
}

std::ostream& operator<<(std::ostream& os, const Handler& handler)
{
	os << "Handler Status Code: " << handler.getStatusCode() << std::endl;
	os << "Handler Response: " << handler.getResponse() << std::endl;
	return os;
}

