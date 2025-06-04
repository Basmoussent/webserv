#include "../includes/ConfigParser.hpp"

std::string ConfigParser::getInstruct(const std::string& key, const Server server) const
{
	std::map<std::string, std::string>::const_iterator it = server.instruct.find(key);
	if (it != server.instruct.end())
		return it->second;
	return "";
}

const std::vector<Server>& ConfigParser::getServers() const
{
	return _servers;
}

const Server& ConfigParser::getServerByPort(int port) const
{
	for (std::size_t i = 0; i < _servers.size(); ++i)
	{
		std::map<std::string, std::string>::const_iterator it = _servers[i].instruct.find("listen");
		if (it != _servers[i].instruct.end())
		{
			int i;
			sscanf(it->second.c_str(), "%d", &i);
			if (i == port)
				return _servers[i];
		}
	}
	throw std::runtime_error("Server not found for the given port.");
}

std::string ConfigParser::trim(const std::string& s)
{
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");
	if (start == std::string::npos || end == std::string::npos)
		return "";
	return s.substr(start, end - start + 1);
}

bool	ConfigParser::WhatIsYourName(const std::string& name, const std::string& val) const
{
	std::cout << "val : " << val << std::endl;
	std::istringstream iss(val);
	std::string next;

	while (iss >> next)
	{
		std::cout << "next name : " << next << std::endl;
		if (next == name)
			return true;
	}
	return false;
}

void ConfigParser::printServers() const
{
	std::cout << "------------- CONFIGURATION -------------\n" << std::endl;
	for (std::size_t s = 0; s < _servers.size(); ++s)
	{
		std::cout << "     --- SERVER " << s + 1 << " ---" << std::endl;

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
		std::cout << "\n-----------------------------------------\n" << std::endl;
	}
}
