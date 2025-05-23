#include "ConfigParser.hpp"

ConfigParser::ConfigParser()
	: _blockDepth(0), _inServer(false), _inLocation(false) {}

ConfigParser::~ConfigParser() {}

bool ConfigParser::parseFile(const std::string& filename) {
	std::ifstream file;
	if (!openFile(filename, file)) {
		std::cerr << "Erreur : impossible d'ouvrir le fichier." << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		size_t pos = line.find('#');

		if (pos != std::string::npos)
			line = line.substr(0, pos);
		line = trim(line);
		if (line.empty())
			continue;
		if (line.back() == ';')
			line.erase(line.size() - 1);
		if (!parseLine(line))
			return false;
	}
	if (this->_blockDepth != 0) {
		std::cerr << "Erreur : bloc non fermé correctement." << std::endl;
		return false;
	}
	return true;
}

bool ConfigParser::openFile(const std::string& filename, std::ifstream& file) {
	file.open(filename.c_str());
	return file.is_open();
}

bool ConfigParser::parseLine(const std::string& line) {
	std::istringstream iss(line);
	std::string word;
	iss >> word;

	if (word == "server") {
		return handleServerBlock();
	} else if (word == "location") {
		return handleLocationBlock(iss);
	} else if (word == "}") {
		return handleClosingBrace();
	} else {
		assignKeyValue(word, iss);
	}
	return true;
}

bool ConfigParser::handleServerBlock() {
	if (this->_blockDepth != 0) {
		std::cerr << "Erreur : bloc 'server' mal placé." << std::endl;
		return false;
	}
	this->_currentServer = Server();
	this->_inServer = true;
	this->_blockDepth = 1;
	return true;
}

bool ConfigParser::handleLocationBlock(std::istringstream& iss) {
	if (this->_blockDepth != 1) {
		std::cerr << "Erreur : bloc 'location' en dehors d'un 'server'." << std::endl;
		return false;
	}
	std::string path;
	iss >> path;
	this->_currentLocation = Location();
	this->_currentLocation.path = path;
	this->_inLocation = true;
	this->_blockDepth = 2;
	return true;
}

bool ConfigParser::handleClosingBrace() {
	if (this->_blockDepth == 2) {
		this->_currentServer.locations.push_back(this->_currentLocation);
		this->_inLocation = false;
		this->_blockDepth = 1;
	} else if (this->_blockDepth == 1) {
		this->_servers.push_back(this->_currentServer);
		this->_inServer = false;
		this->_blockDepth = 0;
	} else {
		std::cerr << "Erreur : fermeture de bloc inattendue." << std::endl;
		return false;
	}
	return true;
}

void ConfigParser::assignKeyValue(const std::string& key, std::istringstream& iss) {
	std::string value, word;
	while (iss >> word) {
		if (!value.empty())
			value += " ";
		value += word;
	}
	if (key.empty() || value.empty())
		return;
	if (this->_inLocation)
		this->_currentLocation.instruct[key] = value;
	else if (_inServer)
		this->_currentServer.instruct[key] = value;
}

const std::vector<Server>& ConfigParser::getServers() const {
	return this->_servers;
}

std::string ConfigParser::trim(const std::string& s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");
	if (start == std::string::npos || end == std::string::npos)
		return "";
	return s.substr(start, end - start + 1);
} 

//fonction temporaire
void ConfigParser::printServers() const {
	for (std::size_t s = 0; s < this->_servers.size(); ++s) {
		std::cout << "\n--- SERVER " << s + 1 << " ---" << std::endl;

		// Affichage des directives du server
		std::map<std::string, std::string>::const_iterator it;
		for (it = this->_servers[s].instruct.begin(); it != this->_servers[s].instruct.end(); ++it) {
			std::cout << "  " << it->first << " : " << it->second << std::endl;
		}

		// Affichage des locations associées
		for (std::size_t i = 0; i < this->_servers[s].locations.size(); ++i) {
			std::cout << "\n  Location: " << this->_servers[s].locations[i].path << std::endl;

			std::map<std::string, std::string>::const_iterator lit;
			for (lit = this->_servers[s].locations[i].instruct.begin();
				 lit != this->_servers[s].locations[i].instruct.end(); ++lit) {
				std::cout << "    " << lit->first << " : " << lit->second << std::endl;
			}
		}
	}
}
