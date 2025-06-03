#pragma once

#include "Webserv.hpp"
#include <string>

class ConfigParser;
class Request;
class Server;
class Location;

class Handler {
private:
    Request& _request;
    ConfigParser& _configParser;
    std::string _response;
    int          _statusCode;
    bool         _isValid;

    static std::string intToString(int value);
    static std::string sizeToString(size_t value);

    std::string getUploadDirectory(const Server& server, const Location& location) const;
    bool handleFileUpload(const std::string& uploadDir, const std::string& filename, const std::string& content);
    std::string extractFilenameFromMultipart(const std::string& body, const std::string& boundary) const;
    std::string extractContentFromMultipart(const std::string& body, const std::string& boundary) const;

    std::string getMimeType(const std::string& path) const;
    std::string buildResponse(int statusCode, const std::string& content, const std::string& contentType) const;
    std::string getCurrentDate() const;
    std::string generateDirectoryListing(const std::string& path, const std::string& uri) const;
    std::string getErrorPage(int statusCode) const;

public:
    Handler(Request& request, ConfigParser& configParser);
    ~Handler();

    void process();
    void handleGet(Server serv, int j);
    void handlePost(std::string& path);
    void handleDelete();
    void handleCGI();
    std::string normalizePath(const std::string& path);
    // Getters
    int getStatusCode() const;
    const std::string& getResponse() const;
    std::string getResponse();
    Request& getRequest();
    bool isValid() const;

    // Setters
    void setStatusCode(int statusCode);
    void setResponse(const std::string& response);
    void setValid(bool isValid);

    void clear();

    void handleHead(Server serv, int j);

};
std::ostream& operator<<(std::ostream& os, const Handler& handler);

