#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <string>
#include "Request.hpp"
#include "ConfigParser.hpp"

class Handler {
private:
    Request      _request;
    std::string  _response;
    int          _statusCode;
    bool         _isValid;
    ConfigParser _configParser;

    static std::string intToString(int value);
    static std::string sizeToString(size_t value);

    // Méthodes privées pour l'upload
    std::string getUploadDirectory(const Server& server, const Location& location) const;
    bool handleFileUpload(const std::string& uploadDir, const std::string& filename, const std::string& content);
    std::string extractFilenameFromMultipart(const std::string& body, const std::string& boundary) const;
    std::string extractContentFromMultipart(const std::string& body, const std::string& boundary) const;

public:
    Handler(const Request& req, const ConfigParser& configParser);
    ~Handler();

    void process();
    void handleGet(Server serv, int j);
    void handlePost(std::string& path);
    void handleDelete(std::string &path);
    void handleCGI();

    // Getters
    int getStatusCode() const;
    const std::string& getResponse() const;
    std::string getResponse();
    bool isValid() const;

    // Setters
    void setStatusCode(int statusCode);
    void setResponse(const std::string& response);
    void setValid(bool isValid);

};
std::ostream& operator<<(std::ostream& os, const Handler& handler);

#endif
