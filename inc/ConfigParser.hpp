#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include <cstdlib>
#include "ConfigValidator.hpp"
#include "ConfigTypes.hpp"

class ConfigParser {
	private :
		int		_blockDepth;
		bool	_inServer;
		bool	_inLocation;
		bool	_bracket;

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
		bool	assignKeyValue(const std::string& key, std::istringstream& iss);

		
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