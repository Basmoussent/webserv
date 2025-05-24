#include "../includes/ConfigParser.hpp"
#include <unistd.h>

bool ConfigParser::isValueValid(const std::string& key, const std::string& value) const {
	if (key == "listen")
		return isValidPort(value);

	if (key == "host")
		return isValidIP(value);

	if (key == "autoindex" || key == "cgi")
		return (value == "on" || value == "off");

	if (key == "client_max_body_size")
		return isInteger(value);

	// if (key == "allow_methods")
	// 	return areValidMethods(value);

	if (key == "cgi_ext")
		return isValidExtension(value);

	if (key == "error_page")
		return isValidErrorPage(value);

	// if (key == "index")
	// 	return areWords(value);

	if (key == "root" || key == "upload_dir" || key == "cgi_path")
		return isValidPath(value); // optionnel : vérifier avec access(value.c_str(), F_OK)

	return true;
}

bool ConfigParser::isValidPath(const std::string& path) const
{
	if (path.empty())
		return false;

	if (access(path.c_str(), F_OK) == 0)
		return true;
	return false;
}

bool ConfigParser::isValidExtension(const std::string& s) const
{
	if (s.empty())
		return false;

	const std::string ext[] = {".py", ".sh"};
	for (size_t i = 0; i < sizeof(ext); ++i)
	{
		if (s == ext[i])
			return true;
	}
	return false;

}

bool ConfigParser::isInteger(const std::string& s) const
{
	if (s.empty())
		return false;

	for (size_t i = 0; i < s.size(); ++i)
	{
		if (!isdigit(s[i]))
			return false;
	}
	return true;
}

bool ConfigParser::isValidPort(const std::string& s) const
{
	if (!isInteger(s))
		return false;
	int port;

	port = std::atoi(s.c_str());
	if (port >= 0 && port <= 65535)
		return true;
	return false;
}

bool ConfigParser::isValidIP(const std::string& ip) const
{
	std::istringstream iss(ip);
	std::string token;
	int count = 0;

	while (std::getline(iss, token, '.'))
	{
		if (!isInteger(token))
			return false;
		int num;

		num = std::atoi(token.c_str());
		if (num < 0 || num > 255)
			return false;
		count++;
	}
	return count == 4;
}

bool ConfigParser::isValidErrorPage(const std::string& val) const
{
	std::istringstream iss(val);
	std::string codeStr;
	std::string path;
	
	iss >> codeStr >> path;
	if (!isInteger(codeStr))
	return false;
	int code;
	
	code = std::atoi(codeStr.c_str());
	if ((code >= 400 && code < 600) && !path.empty() && access(path.c_str(), F_OK) == 0)
		return true;
	return false;
}

// bool ConfigParser::areValidMethods(const std::string& val) const
// {
// 	std::istringstream iss(val);
// 	std::string method;
// 	const std::string valid[] = {"GET", "POST", "DELETE"};
// 	int i;

// 	while (iss >> method)
// 	{
// 		if (valid[i] == method)
// 			return true;
// 	}
// 	return false;
// }
