#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>

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
    const std::map<std::string, std::string>  getFullHeader() const;
    const std::string&  getBody() const;
    bool                isValid() const;

    // Setters
    void setMethod(const std::string method);
    void setUri(const std::string uri);
    void setHttpVersion(const std::string httpVersion);
    void setHeader(const std::string key, const std::string value);
    void setBody(const std::string body);
    void setValid(bool isValid);

    // Parsers
	void parseRequest(const std::string raw_request);
	void parseBody(std::istringstream &request_stream, std::string &body_section);
	void parseRequestLine(const std::string request_line);
    void parseHeaders(const std::string headers_sectionvoid);
    
    void clear();
    // bool validateRequest() const;
};

std::ostream& operator<<(std::ostream& os, const Request& request);

#endif
