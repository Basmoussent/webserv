#include "Handler.hpp"
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

Handler::Handler(const Request& req, const ConfigParser &ConfgiParser) : _request(req), _isValid(false), _configParser(ConfgiParser)
{
	process();
    std::cout << _response << std::endl;
}

Handler::~Handler()
{
}

void Handler::process()
{
	if (_request.isValid())
	{
        size_t i = 0;
        Server server;

        if (_configParser.getServers().empty())
        {
            setStatusCode(500);
            _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
            return;
        }
        if (_configParser.getServers()[0].locations.empty())
        {
            setStatusCode(500);
            _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
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
            _response = "HTTP/1.1 404 Not Found 9\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nNot Found";
            return;
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
                    setStatusCode(405);
                    _response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/plain\r\nContent-Length: 19\r\n\r\nMethod Not Allowed";
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
            _response = "HTTP/1.1 404 Not Found 9\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nNot Found";
            return;
        }

        if (_request.getUri() == "/cgi-bin")
            handleCGI();
        else if (_request.getMethod() == "GET")
			handleGet(server, j);
		else if (_request.getMethod() == "POST")
			handlePost(server.locations[j].path);
		else if (_request.getMethod() == "DELETE")
			handleDelete(server.locations[j].path);
		setValid(true);
	}
	else
	{
		_response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nBad Request";
		setStatusCode(400);
		setValid(false);
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
        _response = "HTTP/1.1 404 Not Found 4\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nNot Found";
        return;
    }

    std::string cgiPath = location.instruct["cgi_path"];
    std::string cgiExt = location.instruct["cgi_ext"];
    std::string index = location.instruct["index"];

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

    std::string filename = "./cgi-bin/" + _request.getUri().substr(_request.getUri().find_last_of("/") + 1);
    std::cout << "Filename: " << filename << std::endl;
    struct stat fileStat;

    if (stat(filename.c_str(), &fileStat) != 0 && !index.empty()) {
        filename = "./cgi-bin/" + index;
        if (stat(filename.c_str(), &fileStat) != 0) {
            setStatusCode(404);
            _response = "HTTP/1.1 404 Not Found 5\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nNot Found";
            return;
        }
    } else if (stat(filename.c_str(), &fileStat) != 0) {
        setStatusCode(404);
        _response = "HTTP/1.1 404 Not Found 6\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nNot Found";
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
        _response = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/plain\r\nContent-Length: 10\r\n\r\nForbidden";
        return;
    }

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
        _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        setStatusCode(500);
        _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        setStatusCode(500);
        _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
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
        _response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: " + len.str() + "\r\n"
                   "\r\n" + output;
    } else {
        setStatusCode(500);
        _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
    }
}

void Handler::handleGet(Server serv, int j)
{
    struct stat buffer;
    std::string root;
    std::string index;

    if (!serv.instruct["root"].empty())
        root = serv.instruct["root"];
    else if (!serv.locations[j].instruct["root"].empty())
        root = serv.locations[j].instruct["root"];

    if (!serv.locations[j].instruct["index"].empty())
        index = serv.locations[j].instruct["index"];
    else if (!serv.instruct["index"].empty())
        index = serv.instruct["index"];

    std::string uri = root + _request.getUri();
    std::string uri_index;

    if (!_request.getUri().empty() && _request.getUri()[_request.getUri().size()] == '/')
        uri_index = root + _request.getUri() + index;
    else if (serv.locations[j].path == "/" && _request.getUri() == "/")
        uri_index = root + "/" + index;
    else
        uri_index = "";
    if (stat(uri.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode))
    {
        setStatusCode(200);
        std::ifstream file(uri.c_str());
        if (file)
        {
            std::ostringstream ss;
            ss << file.rdbuf();
            std::ostringstream len;
            len << ss.str().size();
            _response = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: " + len.str() + "\r\n"
                        "\r\n" + ss.str();
        }
        else
        {
            setStatusCode(500);
            _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
        }
    }
    else if (!uri_index.empty() && stat(uri_index.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode))
    {
        setStatusCode(200);
        std::ifstream file(uri_index.c_str());
        if (file)
        {
            std::ostringstream ss;
            ss << file.rdbuf();
            std::ostringstream len;
            len << ss.str().size();
            _response = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: " + len.str() + "\r\n"
                        "\r\n" + ss.str();
        }
        else
        {
            setStatusCode(500);
            _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
        }
    }
    else if (serv.locations[j].instruct["autoindex"] == "on" && serv.locations[j].path == _request.getUri())
    {
        setStatusCode(200);
        std::string autoindexHtml = "<html><body><h1>Directory Listing</h1><ul>";
        DIR *dir = opendir(("." + _request.getUri()).c_str());
        if (dir)
        {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL)
            {
                if (entry->d_name[0] != '.')
                {
                    autoindexHtml += "<li><a href=\"" + std::string(entry->d_name) + "\">" + std::string(entry->d_name) + "</a></li>";
                }
            }
            closedir(dir);
        }
        autoindexHtml += "</ul></body></html>";
        _response = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: " + sizeToString(autoindexHtml.size()) + "\r\n"
                        "\r\n" + autoindexHtml;
    }
    else
    {
        setStatusCode(404);
        std::ifstream errorFile("./html/404.html");
        std::cout << serv.locations[j].path << std::endl;
        std::cout << _request.getUri() << std::endl;
        if (errorFile)
        {
            std::ostringstream ss;
            ss << errorFile.rdbuf();
            std::ostringstream len;
            len << ss.str().size();
            _response = "HTTP/1.1 404 Not Found 3\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: " + len.str() + "\r\n"
                        "\r\n" + ss.str();
        }
        else
        {
            _response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nNot Found";
        }
    }
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
    
    return "./uploads_default";
}

bool Handler::handleFileUpload(const std::string& uploadDir, const std::string& filename, const std::string& content) {
    struct stat st;
    if (stat(uploadDir.c_str(), &st) != 0) {
        if (mkdir(uploadDir.c_str(), 0755) != 0) {
            return false;
        }
    }

    std::string filepath = uploadDir + "/" + filename;
    
    std::ofstream file(filepath.c_str(), std::ios::out | std::ios::binary);
    if (!file) {
        return false;
    }
    
    file.write(content.c_str(), content.size());
    file.close();
    return true;
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
        _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
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
        _response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nNot Found";
        return;
    }

    if (_request.getHeader("Content-Type").find("multipart/form-data") != std::string::npos) {
        std::string contentType = _request.getHeader("Content-Type");
        std::cout << "Content-Type: " << contentType << std::endl;
        
        size_t boundaryPos = contentType.find("boundary=");
        if (boundaryPos == std::string::npos) {
            setStatusCode(400);
            _response = "HTTP/1.1 400 Bad Request - No boundary found\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nBad Request";
            return;
        }
        
        std::string boundary = contentType.substr(boundaryPos + 9);
        std::string filename = extractFilenameFromMultipart(body, boundary);
        std::string content = extractContentFromMultipart(body, boundary);
        
        if (filename.empty() || content.empty()) {
            setStatusCode(400);
            _response = "HTTP/1.1 400 Bad Request - Empty filename or content\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nBad Request";
            return;
        }

        std::string uploadDir = getUploadDirectory(server, location);
        if (handleFileUpload(uploadDir, filename, content)) {
            setStatusCode(201);
            std::string location = "Location: " + uploadDir + "/" + filename;
            _response = "HTTP/1.1 201 Created\r\nContent-Type: text/plain\r\nContent-Length: " + sizeToString(location.length()) + "\r\n\r\n" + location;
        } else {
            setStatusCode(500);
            _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
        }
        return;
    }

    std::string pathDir = "." + path;
    struct stat st;
    if (stat(pathDir.c_str(), &st) != 0) {
        if (mkdir(pathDir.c_str(), 0755) != 0) {
            setStatusCode(500);
            _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
            return;
        }
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    std::stringstream filename;
    filename << pathDir << "/post_"
             << (t->tm_year + 1900)
             << std::setw(2) << std::setfill('0') << t->tm_mon + 1
             << std::setw(2) << std::setfill('0') << t->tm_mday
             << "_"
             << std::setw(2) << std::setfill('0') << t->tm_hour
             << std::setw(2) << std::setfill('0') << t->tm_min
             << std::setw(2) << std::setfill('0') << t->tm_sec
             << ".log";

    std::string filepath = filename.str();
    std::cout << "File path: " << filepath << std::endl;
    std::ofstream file(filepath.c_str(), std::ios::out | std::ios::trunc);
    if (file) {
        file << body;
        file.close();
        setStatusCode(201);
        std::string location = "Location: " + filepath;
        _response = "HTTP/1.1 201 Created\r\nContent-Type: text/plain\r\nContent-Length: " + sizeToString(location.length()) + "\r\n\r\n" + location;
    } else {
        setStatusCode(500);
        _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error";
    }
}

void Handler::handleDelete(std::string &path)
{
    const std::string filename = _request.getBody();
    const std::string fullPath = "." + path + "/" + filename;

    if (filename.empty())
    {
        setStatusCode(400);
        _response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 24\r\n\r\nMissing filename in body";
        return;
    }

    if (remove(fullPath.c_str()) == 0)
    {
        setStatusCode(200);
        std::string message = "File deleted successfully: " + filename;
        _response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + sizeToString(message.length()) + "\r\n\r\n" + message;
    }
    else
    {
        setStatusCode(404);
        std::string message = "File not found: " + fullPath;
        _response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: " + sizeToString(message.length()) + "\r\n\r\n" + message;
    }
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

std::ostream& operator<<(std::ostream& os, const Handler& handler)
{
	os << "Handler Status Code: " << handler.getStatusCode() << std::endl;
	os << "Handler Response: " << handler.getResponse() << std::endl;
	return os;
}

