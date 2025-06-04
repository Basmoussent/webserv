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
    std::string                        _query_String;
    std::string                         _host;
    std::string                         _boundary;
    bool                                _multiform;
    bool                                _chunked;
    bool                                _isValid;
    bool                                _headersParsed;
    size_t                              _contentLength;
    std::string                         _rawRequest;

public:
    Request();
    Request(const std::string& raw_request);
    Request(const Request& other);
    ~Request();
    Request& operator=(const Request& other);

    Request& getRequest();

    // Getters
    const std::string&  getMethod() const;
    const std::string&  getUri() const;
    const std::string&  getHttpVersion() const;
    const std::string&  getHeader(const std::string& key) const;
    const std::map<std::string, std::string>  getFullHeader() const;
    const std::string&  getBody() const;
    const std::string&  getQueryString() const;
    const std::string&  getHost() const;
    bool                isValid() const;
    bool                isComplete() const;
    const std::string&  getRawRequest() const;
    

    // Setters
    void setMethod(const std::string method);
    void setUri(const std::string uri);
    void setHttpVersion(const std::string httpVersion);
    void setHeader(const std::string key, const std::string value);
    void setBody(const std::string& body);
    void setQueryString(const std::string queryString);
    void setValid(bool isValid);

    void feed(const char* buffer, size_t bytes_read);

    // Parsers
    void parseRequest(const std::string raw_request);
    void parseBody(std::istringstream &request_stream, std::string &body_section);
    void parseRequestLine(const std::string request_line);
    void parseHeaders(const std::string headers_section);
    void appendBody(const std::string& additional_data);
    
    void clear();
    // bool validateRequest() const;
    
    // Fonctions pour le traitement des requêtes multipart/form-data
    std::string extractContentFromMultipart(const std::string& body, const std::string& boundary) const;
    std::string extractFilenameFromMultipart(const std::string& body, const std::string& boundary) const;
};

std::ostream& operator<<(std::ostream& os, const Request& request);

#endif
