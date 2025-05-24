#include "Handler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <filesystem>

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
            _response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            return;
        }
        if (_configParser.getServers()[0].locations.empty())
        {
            setStatusCode(500);
            _response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            return;
        }
        for (i = 0; i < _configParser.getServers().size(); i++)
        {
            server = _configParser.getServers()[i];
            if (_request.getHeader("Host") == server.instruct["host"] || _request.getHeader("Host") == server.instruct["server_name"])
                break;
        }
        if (i == _configParser.getServers().size())
        {
            setStatusCode(404);
            _response = "HTTP/1.1 404 Not Found 1\r\n\r\n";
            return;
        }
        bool locationFound = false;
        size_t j;
        
        for (j = 0; j < server.locations.size(); ++j)
        {
            if (server.locations[j].path == _request.getUri())
            {
                std::string allowedMethods = server.locations[j].instruct["allow_methods"];
                if (allowedMethods.find(_request.getMethod()) == std::string::npos)
                {
                    setStatusCode(405);
                    _response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
                    return;
                }
                locationFound = true;
                break;
            }
        }

        if (!locationFound)
        {
            setStatusCode(404);
            _response = "HTTP/1.1 404 Not Found 2\r\n\r\n";
            return;
        }

        if (_request.getMethod() == "GET")
			handleGet();
		if (_request.getMethod() == "POST")
			handlePost(server.locations[j].path);
		if (_request.getMethod() == "DELETE")
			handleDelete(server.locations[j].path);
		setValid(true);
	}
	else
	{
		_response = "HTTP/1.1 400 Bad Request Method Not Available\r\n";
		setStatusCode(400);
		setValid(false);
	}
}

void Handler::handleGet()
{
    struct stat buffer;
    const std::string& uri = "." + _request.getUri();

    printf("%s\n", uri.c_str());
	if (stat(uri.c_str(), &buffer) == 0)
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
            _response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        }
    }
    else
    {
        setStatusCode(404);
        std::ifstream errorFile("./html/error.html");
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
            _response = "HTTP/1.1 404 Not Found\r\n\r\n404 Error";
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
        _response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        return;
    }

    Server server;
    Location location;
    bool found = false;
    
    for (size_t i = 0; i < _configParser.getServers().size(); i++) {
        server = _configParser.getServers()[i];
        if (_request.getHeader("Host") == server.instruct["host"] || 
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
        _response = "HTTP/1.1 404 Not Found\r\n\r\n";
        return;
    }

    if (_request.getHeader("Content-Type").find("multipart/form-data") != std::string::npos) {
        std::string contentType = _request.getHeader("Content-Type");
        std::cout << "Content-Type: " << contentType << std::endl;
        
        size_t boundaryPos = contentType.find("boundary=");
        if (boundaryPos == std::string::npos) {
            setStatusCode(400);
            _response = "HTTP/1.1 400 Bad Request - No boundary found\r\n\r\n";
            return;
        }
        
        std::string boundary = contentType.substr(boundaryPos + 9);
        std::string filename = extractFilenameFromMultipart(body, boundary);
        std::string content = extractContentFromMultipart(body, boundary);
        
        if (filename.empty() || content.empty()) {
            setStatusCode(400);
            _response = "HTTP/1.1 400 Bad Request - Empty filename or content\r\n\r\n";
            return;
        }

        std::string uploadDir = getUploadDirectory(server, location);
        if (handleFileUpload(uploadDir, filename, content)) {
            setStatusCode(201);
            _response = "HTTP/1.1 201 Created\r\nLocation: " + uploadDir + "/" + filename + "\r\n\r\n";
        } else {
            setStatusCode(500);
            _response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        }
        return;
    }

    std::string pathDir = "." + path;
    struct stat st;
    if (stat(pathDir.c_str(), &st) != 0) {
        if (mkdir(pathDir.c_str(), 0755) != 0) {
            setStatusCode(500);
            _response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
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
        _response = "HTTP/1.1 201 Created\r\nLocation: ";
        _response += filepath;
        _response += "\r\n\r\n";
    } else {
        setStatusCode(500);
        _response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
    }
}

void Handler::handleDelete(std::string &path)
{
	const std::string& body = "." + path + "/" + _request.getBody();
	if (remove(body.c_str()) == 0)
	{
		setStatusCode(200);
		_response = "HTTP/1.1 200 OK\r\n\r\n";
	}
	else
	{
		setStatusCode(404);
		_response = "HTTP/1.1 404 Not Found 4\r\n\r\n";
        _response += "File not found: ";
        _response += body;
        _response += "\r\n";
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

