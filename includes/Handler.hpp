#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <string>
#include "Request.hpp"

class Handler {
private:
    Request      _request;
    std::string  _response;
    int          _statusCode;
    bool         _isValid;

public:
    Handler(const Request& req);
    ~Handler();

    void process();

    // Getters
    const std::string& getResponse() const;
    int getStatusCode() const;
    bool isValid() const;
};

#endif // HANDLER_HPP
