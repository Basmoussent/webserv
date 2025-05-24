#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <sstream>

struct Location {
	std::string path;
	std::map<std::string, std::string> instruct;
};

struct Server {
	std::vector<Location> locations;
	std::map<std::string, std::string> instruct;
	// const std::string& operator[](const std::string& key) const {
    //     static const std::string empty = "";
    //     std::map<std::string, std::string>::const_iterator it = instruct.find(key);
    //     return it != instruct.end() ? it->second : empty;
    // }
};

class ConfigParser {
	private :
		int		_blockDepth;
		bool	_inServer;
		bool	_inLocation;
		std::string	test;

		std::vector<Server>	_servers;
		Server				_currentServer;
		Location			_currentLocation;

		std::map<std::string, int>	_keywords;
		int							_mandatoryCount;

		bool	openFile(const std::string& filename, std::ifstream& file);
		bool	parseLine(const std::string& line);
		bool	handleServerBlock();
		bool	handleLocationBlock(std::istringstream& iss);
		bool	handleClosingBrace();
		void	assignKeyValue(const std::string& key, std::istringstream& iss);

		
		std::string	trim(const std::string& s);

		bool	isMandatory(const std::string& key);

		std::string getInstruct(const std::string& key, const Server server) const;
		

	public :
		ConfigParser();
		~ConfigParser();

		bool parseFile(const std::string& filename);
		void printServers() const;
		const std::vector<Server>&	getServers() const;
		Server& getServerByPort(int port) const;

};

#endif