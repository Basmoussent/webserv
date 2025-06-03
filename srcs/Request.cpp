#include "Request.hpp"
#include "stdio.h"
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <vector>

Request::Request() : _isValid(false), _headersParsed(false), _contentLength(0)
{

}

Request::Request(const std::string& raw_request) : _isValid(true), _headersParsed(false), _contentLength(0)
{
	_rawRequest = raw_request;
	parseRequest(raw_request);
}

Request::Request(const Request& other)
{
	*this = other;
}

Request::~Request()
{

}

Request& Request::getRequest()
{
	return *this;
}

Request& Request::operator=(const Request& other)
{
	if (this != &other)
	{
		_method = other._method;
		_uri = other._uri;
		_httpVersion = other._httpVersion;
		_headers = other._headers;
		_body = other._body;
		_isValid = other._isValid;
	}
	return *this;
}

void Request::feed(char *buffer, size_t bytes_read)
{
	_rawRequest.append(buffer, bytes_read);
	if (!_headersParsed)
	{
		size_t pos = _rawRequest.find("\r\n\r\n");
		if (pos != std::string::npos)
		{
			std::string headers_section = _rawRequest.substr(0, pos + 2);
			parseHeaders(headers_section);
			_headersParsed = true;
			_rawRequest.erase(0, pos + 4);
		}
	}
	if (_headersParsed && !_isValid)
	{
		std::istringstream stream(_rawRequest);
		parseBody(stream, _body);		
	}
}

// Setters
void Request::setMethod(const std::string method)
{
	if (method == "GET" || method == "POST" || method == "DELETE" || method == "HEAD")
		_method = method;
	else
	{
		_method = "UNKNOWN";
		std::cout << "Invalid method: " << method << std::endl;
		setValid(false);
	}	
}

void Request::setUri(const std::string uri)
{
	_uri = uri;
}

void Request::setHttpVersion(const std::string httpVersion)
{
	_httpVersion = httpVersion;
	if (httpVersion != "HTTP/1.1")
	{
		std::cout << "Invalid HTTP version: " << httpVersion << std::endl;
		setValid(false);
	}
}

void Request::setHeader(const std::string key, const std::string value)
{
	size_t start = value.find_first_not_of(" \t\r\n");
	size_t end = value.find_last_not_of(" \t\r\n");
	_headers[key] = value.substr(start, end - start + 1);
}

void Request::setBody(const std::string body)
{
	if (_method == "DELETE") {
		size_t start = body.find_first_not_of(" \t\r\n");
		size_t end = body.find_last_not_of(" \t\r\n");
		if (start != std::string::npos && end != std::string::npos)
			_body = body.substr(start, end - start + 1);
		else
			_body = body;
	} else if (_method == "POST" && _multiform) {
		_body += body;
	} else {
		size_t start = body.find_first_not_of(" \t\r");
		size_t end = body.find_last_not_of(" \t\r");
		if (start != std::string::npos && end != std::string::npos)
			_body += body.substr(start, end - start + 1);
		else
			_body += body;
	}
}

void Request::setValid(bool isValid)
{
	_isValid = isValid;
}

void Request::setQueryString(const std::string queryString)
{
	_query_String = queryString;
}

// Getters
const std::string& Request::getMethod() const
{
	return _method;
}

const std::string& Request::getUri() const
{
	return _uri;
}

const std::string& Request::getHttpVersion() const
{
	return _httpVersion;
}

const std::string& Request::getQueryString() const
{
	return _query_String;
}

const std::string& Request::getHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
        return it->second;
    static const std::string empty_string;
    return empty_string;
}

const std::map<std::string, std::string> Request::getFullHeader() const
{
	return _headers;
}

const std::string& Request::getBody() const
{
	return _body;
}

const std::string& Request::getHost() const
{
	return _host;
}

bool Request::isValid() const
{
	return _isValid;
}

void Request::parseRequest(const std::string raw_request)
{
	std::istringstream request_stream(raw_request);
	// getLine works with string_stream so we can't pass the raw string to it
	std::string request_line;
	std::string header_line; 
	std::string headers_section;
	std::string body_section;

	if (std::getline(request_stream, request_line) && !request_line.empty())
		parseRequestLine(request_line);
	while (std::getline(request_stream, header_line))
	{
		if (header_line == "\r" || header_line.empty())
			break ;
		headers_section+=header_line; 
		headers_section += "\n";
	}
	parseHeaders(headers_section);
	parseBody(request_stream, body_section);
	if (!_chunked && getHeader("Host").empty())
	{
		std::cout << "Missing Host header in request" << std::endl;
		setValid(false);	
	}
}

void Request::parseBody(std::istringstream &request_stream, std::string &body_section)
{
    if (_chunked) {
        std::string line;
        while (std::getline(request_stream, line)) {
            std::cout << "Parsing chunked body line: [" << line << "]" << std::endl;
            if (!line.empty() && line[line.length() - 1] == '\r') {
                line = line.substr(0, line.length() - 1);
            }
            if (line.empty()) {
                continue;
            }
            size_t chunk_size;
            try {
                chunk_size = std::strtoul(line.c_str(), NULL, 16);
                std::cout << "Chunk size: " << chunk_size << std::endl;
            } catch (const std::exception&) {
                std::cout << "Failed to parse chunk size from: " << line << std::endl;
                continue;
            }
            if (chunk_size == 0) {
                std::cout << "Found end chunk" << std::endl;
                break;
            }
            std::string chunk;
            chunk.resize(chunk_size);
            request_stream.read(&chunk[0], chunk_size);
            char crlf[2];
            request_stream.read(crlf, 2);
            std::cout << "Read chunk content: [" << chunk << "]" << std::endl;
            body_section += chunk;
        }
    } else {
        std::string line;
        bool isFirstLine = true;
        
        while (std::getline(request_stream, line)) {
            if (isFirstLine && line.empty()) {
                isFirstLine = false;
                continue;
            }
            if (!line.empty()) {
                if (!body_section.empty())
                    body_section += "\n";
                body_section += line;
            }
        }
    }
    setBody(body_section);
	std::cout << "Parsed body: [" << this->_body << "]" << std::endl;

	if (body_section.empty()) {
		_contentLength = 0;
	} else {
		std::map<std::string, std::string>::const_iterator it = _headers.find("Content-Length");
		if (it != _headers.end()) {
			_contentLength = static_cast<size_t>(atoi(it->second.c_str()));
		} else {
			_contentLength = body_section.length();
		}
	}
}

void Request::parseRequestLine(const std::string request_line)
{
	std::istringstream line_stream(request_line);
	std::string method;
	std::string uri;
	std::string http_version;

	if (line_stream >> method >> uri >> http_version)
	{
		setMethod(method);
		if(uri.find('?') != std::string::npos)
		{
			std::size_t pos = uri.find('?');
			setQueryString(uri.substr(pos + 1));
			uri = uri.substr(0, pos);
		}
		setUri(uri);
		setHttpVersion(http_version);
	}
	else
	{
		std::cout << "Invalid request line: " << request_line << std::endl;
		setValid(false);
	}
}

void Request::parseHeaders(std::string header)
{
    std::istringstream header_stream(header);
    std::string line;

    while (std::getline(header_stream, line))
    {
        std::size_t pos = line.find(':');
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Trim whitespace from key and value
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);

            if (key == "Content-Type" && value.find("multipart/form-data") != std::string::npos)
            {
                std::size_t boundary_pos = value.find("boundary=");
                if (boundary_pos != std::string::npos)
                {
                    _boundary = value.substr(boundary_pos + 9);
                    _multiform = true;
                }
            }
            else if (key == "Transfer-Encoding" && value.find("chunked") != std::string::npos) {
                _chunked = true;
                std::cout << "Found chunked transfer encoding" << std::endl;
            }
            setHeader(key, value);
        }
    }
}

std::ostream& operator<<(std::ostream& os, const Request& request)
{
	os << "Method: " << request.getMethod() << "\n";
	os << "URI: " << request.getUri() << "\n";
	if (!request.getQueryString().empty())
		os << "Query String: " << request.getQueryString() << "\n";
	os << "HTTP Version: " << request.getHttpVersion() << "\n";
	os << "Headers:\n";

	std::map<std::string, std::string> headers = request.getFullHeader();
	std::map<std::string, std::string>::const_iterator it;
	for (it = headers.begin(); it != headers.end(); ++it)
	{
		os << it->first << ": " << it->second << "\n";
	}
	os << "Body: \n" << request.getBody();
	if (request.isValid())
		os << "\nRequest is valid.\n";
	else
		os << "\nRequest is invalid.\n";
	return os;
}

void Request::appendBody(const std::string& additional_data) {
	_rawRequest += additional_data;
	
	// Si les headers n'ont pas encore été parsés, essayer de les parser
	if (!_headersParsed) {
		size_t header_end = _rawRequest.find("\r\n\r\n");
		if (header_end != std::string::npos) {
			std::string headers_section = _rawRequest.substr(0, header_end);
			parseHeaders(headers_section);
			_headersParsed = true;
			
			// Mettre à jour la longueur du contenu
			std::map<std::string, std::string>::const_iterator it = _headers.find("Content-Length");
			if (it != _headers.end()) {
				_contentLength = static_cast<size_t>(atoi(it->second.c_str()));
			}
		}
	}
	
	// Mettre à jour le body
	size_t body_start = _rawRequest.find("\r\n\r\n");
	if (body_start != std::string::npos) {
		_body = _rawRequest.substr(body_start + 4);
	}
}

bool Request::isComplete() const {
	if (!_headersParsed) {
		return false;
	}
	if (_contentLength == 0) {
		return true;
	}
	return _body.length() >= _contentLength;
}

const std::string& Request::getRawRequest() const {
	return _rawRequest;
}

void Request::clear() {
	_method.clear();
	_uri.clear();
	_httpVersion.clear();
	_headers.clear();
	_body.clear();
	_query_String.clear();
	_host.clear();
	_boundary.clear();
	_multiform = false;
	_chunked = false;
	_headersParsed = false;
	_contentLength = 0;
	_rawRequest.clear();
}

// check request type
// Verifier le HOST car obligatoire