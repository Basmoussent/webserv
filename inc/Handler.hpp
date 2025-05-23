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

public:
    Handler(const Request& req, const ConfigParser& configParser);
    ~Handler();

    void process();
    void handleGet();
    void handlePost(std::string& path);
    void handleDelete(std::string &path);

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
