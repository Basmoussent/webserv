#include "../includes/ConfigParser.hpp"

bool ConfigParser::isValueValid(const std::string& key, const std::string& value) const
{
	if (key == "listen")
		return isValidPort(value);

	if (key == "host")
		return isValidIP(value);

	if (key == "server_name")
		return isName(value);

	if (key == "autoindex" || key == "cgi")
		return (value == "on" || value == "off");

	if (key == "client_max_body_size")
		return isInteger(value);

	if (key == "allow_methods")
		return ValidMethods(value);

	if (key == "cgi_ext")
		return isValidExtension(value);

	if (key == "error_page")
		return isValidErrorPage(value);

	if (key == "index")
		return isValidIndex(value);

	if (key == "root" || key == "upload_dir" || key == "cgi_path")
		return isValidPath(value);

	return true;
}

bool	ConfigParser::isName(const std::string& name) const
{
	for(size_t i = 0; i < name.size() ; ++i)
	{
		char c = name[i];
		if(!isalnum(c) && c == '.')
			return false;
	}
	return true;
}

bool ConfigParser::isValidPath(const std::string& path) const
{
	if (access(path.c_str(), F_OK) == 0)
		return true;
	return false;
}

bool ConfigParser::isValidExtension(const std::string& s) const
{
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

	port = atoi(s.c_str());
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

		num = atoi(token.c_str());
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
	
	code = atoi(codeStr.c_str());
	if ((code >= 400 && code < 600) && !path.empty() && access(path.c_str(), F_OK) == 0)
		return true;
	return false;
}

bool ConfigParser::ValidMethods(const std::string& val) const
{
	std::istringstream iss(val);
	std::string method;
	
	while (iss >> method)
	{
		if (method != "GET" && method != "POST" && method != "DELETE")
			return false;
	}
	return true;
}

bool	ConfigParser::isValidIndex(const std::string& val) const
{
	std::istringstream iss(val);
	std::string index;
	std::string path;

	while (iss >> index)
	{
		path = "/var/www/html/" + index;
		//a verifier
		// if (access(path.c_str(), F_OK) == 0)
			return true;
	}
	return false;
}

// fonction que je vais enlever permet seulement de faire les checks
void ConfigParser::printServers() const
{
	for (std::size_t s = 0; s < _servers.size(); ++s)
	{
		std::cout << "\n--- SERVER " << s + 1 << " ---" << std::endl;

		std::map<std::string, std::string>::const_iterator it;
		for (it = _servers[s].instruct.begin(); it != _servers[s].instruct.end(); ++it)
		{
			std::cout << "  " << it->first << " : " << it->second << std::endl;
		}

		for (std::size_t i = 0; i < _servers[s].locations.size(); ++i)
		{
			std::cout << "\n  Location: " << _servers[s].locations[i].path << std::endl;

			std::map<std::string, std::string>::const_iterator lit;
			for (lit = _servers[s].locations[i].instruct.begin();
					lit != _servers[s].locations[i].instruct.end(); ++lit)
				{
				std::cout << "    " << lit->first << " : " << lit->second << std::endl;
			}
		}
	}
}