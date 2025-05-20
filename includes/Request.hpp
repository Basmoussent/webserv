#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include <vector>

class Request {
private:
    std::string                         _method;
    std::string                         _uri;
    std::string                         _httpVersion;
    std::map<std::string, std::string>  _headers;
    std::string                         _body;
    bool                                _isValid;

public:
    Request();
    Request(const std::string& raw_request);
    Request(const Request& other);
    ~Request();
    Request& operator=(const Request& other);

    // Getters
    const std::string&  getMethod() const;
    const std::string&  getUri() const;
    const std::string&  getHttpVersion() const;
    const std::string&  getHeader(const std::string& key) const;
    const std::string&  getBody() const;
    bool                isValid() const;

    void parseRequest(const std::string& raw_request);
    void parseRequestLine(const std::string& request_line);
    void parseHeaders(const std::string& headers_section);
    void parseBody(const std::string& body_section);

    void clear();
    bool validateRequest() const;
};

#endif
