#include "Webserv.hpp"
#include <sys/stat.h>


ConfigParser::ConfigParser()
	: _blockDepth(0), _inServer(false), _inLocation(false)
{
	_keywords["listen"] = 1;
	_keywords["root"] = 1;
	_keywords["host"] = 1;
	_keywords["index"] = 1;
	_keywords["location"] = 0;
	_keywords["server_name"] = 0;
	_keywords["autoindex"] = 0;
	_keywords["error_page"] = 0;
	_keywords["client_max_body_size"] = 0;
	_keywords["allow_methods"] = 0;
	_keywords["cgi"] = 0;
	_keywords["cgi_path"] = 0;
	_keywords["cgi_ext"] = 0;
	_keywords["return"] = 0;
}

ConfigParser::~ConfigParser() {}

bool	ConfigParser::parseFile(const std::string& filename)
{
	std::ifstream file;
	if (!openFile(filename, file))
	{
		std::cerr << "Error: unable to open file." << std::endl;
		return false;
	}
	struct stat sb;
	if (stat(filename.c_str(), &sb) == -1)
	{
		std::cerr << "Error: unable to get file status." << std::endl;
		return false;
	}
	if (!S_ISREG(sb.st_mode))
	{
		std::cerr << "Error: conf file is not a regular file." << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line))
	{
		size_t pos = line.find('#');

		if (pos != std::string::npos)
			line = line.substr(0, pos);
		line = trim(line);
		if (line.empty())
			continue;
		if (line[line.size() - 1 ] == ';')
			line.erase(line.size() - 1);
		if (!parseLine(line))
			return false;
	}
	if (!checkMinimumConfig()) 
	{
		std::cerr << "Error: minimum configuration not met." << std::endl;
		return false;
	}
	if (_blockDepth != 0)
	{
		std::cerr << "Error: block not closed correctly." << std::endl;
		return false;
	}

	return true;
}

bool	ConfigParser::openFile(const std::string& filename, std::ifstream& file)
{
	file.open(filename.c_str());
	return file.is_open();
}

bool	ConfigParser::parseLine(const std::string& line)
{
	std::istringstream iss(line);
	std::string word;
	iss >> word;

	if (word == "server")
	{
		iss >> word;
		if (word == "{") 
			_inServer = true;
		return handleServerBlock();
	}
	else if (word == "location")
		return handleLocationBlock(iss);
	else if (word == "}")
		return handleClosingBrace();
	else if (word == "{")
	{
		if (_inServer == false && _blockDepth == 1)
			_inServer = true;
		if (_inLocation == false && _blockDepth == 2)
			_inLocation = true;
	}
	else if (_keywords.find(word) == _keywords.end())
	{
		std::cerr << "Error: unknown keyword '" << word << "'." << std::endl;
		return false;
	}
	else
	{
		if ((_inServer == false && _blockDepth == 1) || (_inLocation == false && _blockDepth == 2))
		{
			std::cerr << "Error: no opening bracket" << std::endl;
			return false;
		}
		if (!assignKeyValue(word, iss))
			return false;
	}
	return true;
}


bool	ConfigParser::handleServerBlock()
{
	if (_blockDepth != 0)
	{
		std::cerr << "Error: misplaced 'server' block." << std::endl;
		return false;
	}
	_currentServer = Server();
	_blockDepth = 1;
	return true;
}

bool	ConfigParser::handleLocationBlock(std::istringstream& iss)
{
	if (_blockDepth != 1)
	{
		std::cerr << "Error: 'location' block outside a 'server'." << std::endl;
		return false;
	}
	std::string path;
	iss >> path;
	
	_currentLocation = Location();
	_currentLocation.path = path;
	iss >> path;
	if (path == "{") 
	_inLocation = true;
	_blockDepth = 2;
	return true;
}

bool	ConfigParser::handleClosingBrace()
{
	if (_blockDepth == 2)
	{
		bool replaced = false;
		for (size_t i = 0; i < _currentServer.locations.size(); ++i)
		{
			if (_currentServer.locations[i].path == _currentLocation.path)
			{
				_currentServer.locations[i] = _currentLocation;
				replaced = true;
				break;
			}
		}
		if (!replaced) {
			_currentServer.locations.push_back(_currentLocation);
		}
		_inLocation = false;
		_blockDepth = 1;
	}
	else if (_blockDepth == 1)
	{
		_servers.push_back(_currentServer);
		_inServer = false;
		_blockDepth = 0;
	}
	else
	{
		std::cerr << "Error: unexpected block closure." << std::endl;
		return false;
	}
	return true;
}

bool	ConfigParser::checkMinimumConfig() const
{
	for (size_t s = 0; s < _servers.size(); ++s)
	{
		const Server& srv = _servers[s];
		for (std::map<std::string, int>::const_iterator it = _keywords.begin(); it != _keywords.end(); ++it)
		{
			if (it->second == 1)
			{
				if (it->first == "listen")
				{
					std::map<std::string, std::string>::const_iterator hostIt = srv.instruct.find("host");
					bool hostHasPort = false;
					if (hostIt != srv.instruct.end())
					{
						const std::string& hostVal = hostIt->second;
						if (hostVal.find(':') != std::string::npos)
							hostHasPort = true;
					}
					if (srv.instruct.find("listen") == srv.instruct.end() && !hostHasPort)
					{
						std::cerr << "Error: missing required directive 'listen' in server block " << s + 1 << "." << std::endl;
						return false;
					}
				}
				else if (srv.instruct.find(it->first) == srv.instruct.end())
				{
					std::cerr << "Error: missing required directive '" << it->first << "' in server block " << s + 1 << "." << std::endl;
					return false;
				}
			}
		}
	}
	return true;
}

bool	ConfigParser::assignKeyValue(const std::string& key, std::istringstream& iss)
{
	std::string value, word;
	while (iss >> word)
	{
		if (!value.empty())
			value += " ";
		value += word;
	}
	if (key.empty() || value.empty())
	{
		std::cerr << "Error: the directive '" << key << "' must have a value." << std::endl;
		return false;
	}
	if (_inLocation)
		_currentLocation.instruct[key] = value;
	else if (_inServer)
		_currentServer.instruct[key] = value;
	return true;
}
