#include "Handler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>

Handler::Handler(const Request& req) : _request(req), _isValid(false)
{
	process();
}

Handler::~Handler()
{
}

void Handler::process()
{
	if (_request.isValid())
	{
		if (_request.getMethod() == "GET")
			handleGet();
		if (_request.getMethod() == "POST")
			handlePost();
		if (_request.getMethod() == "DELETE")
			handleDelete();
		setValid(true);
	}
	else
	{
		_response = "HTTP/1.1 400 Bad Request\r\n";
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
            _response = "HTTP/1.1 404 Not Found\r\n"
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

void Handler::handlePost()
{
    const std::string& body = _request.getBody();

    time_t now = time(NULL);
    std::stringstream filename;

    filename << "./log/";
    
	if (_request.getUri() != "/log" || _request.getUri() != "/log/")
	{
		setStatusCode(500);
        _response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
	}
    struct stat st;
    if (stat("./log", &st) != 0) {
        mkdir("./log", 0755);
    }

    filename << "post_";
    struct tm *t = localtime(&now);
    filename << (t->tm_year + 1900)
             << std::setw(2) << std::setfill('0') << t->tm_mon + 1
             << std::setw(2) << std::setfill('0') << t->tm_mday
             << "_"
             << std::setw(2) << std::setfill('0') << t->tm_hour
             << std::setw(2) << std::setfill('0') << t->tm_min
             << std::setw(2) << std::setfill('0') << t->tm_sec
             << ".log";

    std::string filepath = filename.str();

    std::ofstream file(filepath.c_str(), std::ios::out | std::ios::trunc);
    if (file) {
        file << body;
        file.close();
        setStatusCode(201);
        _response = "HTTP/1.1 201\r\nCreated at ";
		_response += filepath;
		_response += "\r\n\r\n";
    } else {
        setStatusCode(500);
        _response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
    }
}


void Handler::handleDelete()
{
	const std::string& uri = "." +_request.getUri();

	if (remove(uri.c_str()) == 0)
	{
		setStatusCode(200);
		_response = "HTTP/1.1 200 OK\r\n\r\n";
	}
	else
	{
		setStatusCode(404);
		_response = "HTTP/1.1 404 Not Found\r\n\r\n";
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

