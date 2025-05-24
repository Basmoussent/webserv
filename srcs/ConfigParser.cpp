#include "../includes/ConfigParser.hpp"

ConfigParser::ConfigParser()
	: _blockDepth(0), _inServer(false), _inLocation(false)
{
	_keywords["listen"] = 1;
	_keywords["server_name"] = 0;
	_keywords["root"] = 1;
	_keywords["host"] = 1;
	_keywords["index"] = 1;
	_keywords["autoindex"] = 0;
	_keywords["error_page"] = 0;
	_keywords["client_max_body_size"] = 0;
	_keywords["allow_methods"] = 1;
	_keywords["cgi"] = 1;
	_keywords["cgi_path"] = 1;
	_keywords["cgi_ext"] = 1;
	_keywords["upload_dir"] = 0;
}

ConfigParser::~ConfigParser() {}

bool ConfigParser::parseFile(const std::string& filename)
{
	std::ifstream file;
	if (!openFile(filename, file))
	{
		std::cerr << "Error: unable to open file." << std::endl;
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
	if (_blockDepth != 0)
	{
		std::cerr << "Error: block not closed correctly." << std::endl;
		return false;
	}
	return true;
}

bool ConfigParser::openFile(const std::string& filename, std::ifstream& file)
{
	file.open(filename.c_str());
	return file.is_open();
}

bool ConfigParser::parseLine(const std::string& line)
{
	std::istringstream iss(line);
	std::string word;
	iss >> word;

	if (word == "server") {
		iss >> word;
		if (word == "{") 
			_inServer = true;
		return handleServerBlock();
	}
	else if (word == "location") {
		return handleLocationBlock(iss);
	}
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
			std::cout << "Error: no opening bracket" << std::endl;
			return false;
		}
		if (!assignKeyValue(word, iss))
			return false;
	}
	return true;
}

bool ConfigParser::handleServerBlock()
{
	if (_blockDepth != 0)
	{
		std::cerr << "zError: misplaced 'server' block." << std::endl;
		return false;
	}
	_currentServer = Server();
	_blockDepth = 1;
	return true;
}

bool ConfigParser::handleLocationBlock(std::istringstream& iss)
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

bool ConfigParser::handleClosingBrace()
{
	if (_blockDepth == 2)
	{
		_currentServer.locations.push_back(_currentLocation);
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

bool ConfigParser::assignKeyValue(const std::string& key, std::istringstream& iss)
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
	if (!isValueValid(key, value))
	{
		std::cerr << "Error: invalid value for '" << key << "' : '" << value << "'" << std::endl;
		return false;
	}
	if (_inLocation)
		_currentLocation.instruct[key] = value;
	else if (_inServer)
		_currentServer.instruct[key] = value;
	return true;
}

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