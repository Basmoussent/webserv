#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

struct Location
{
	std::string path;
	std::map<std::string, std::string> instruct;
};

struct Server
{
	std::vector<Location> locations;
	std::map<std::string, std::string> instruct;
};

class ConfigParser
{
	private :
		int		_blockDepth;
		bool	_inServer;
		bool	_inLocation;

		std::map<std::string, int>	_keywords;

		std::vector<Server>	_servers;
		Server				_currentServer;
		Location			_currentLocation;

		//parsing de .conf
		bool	openFile(const std::string& filename, std::ifstream& file);
		bool	parseLine(const std::string& line);
		bool	handleServerBlock();
		bool	handleLocationBlock(std::istringstream& iss);
		bool	handleClosingBrace();
		bool	assignKeyValue(const std::string& key, std::istringstream& iss);
		std::string	trim(const std::string& s);
		
		//check des values
		bool	isValueValid(const std::string& key, const std::string& value) const;
		bool	isInteger(const std::string& s) const;
		bool	isValidPort(const std::string& s) const;
		bool	isValidIP(const std::string& ip) const;
		bool	areValidMethods(const std::string& val) const;
		bool	isValidErrorPage(const std::string& val) const;
		bool	isName(const std::string& val) const;
		bool	isValidExtension(const std::string& s) const;
		bool	isValidPath(const std::string& path) const ;

		//jsp c'est Basem
		std::string	getInstruct(const std::string& key, const Server server) const;


	public :
		ConfigParser();
		~ConfigParser();

		bool parseFile(const std::string& filename);
		void printServers() const;

		const std::vector<Server>&	getServers() const;
		const Server& getServerByPort(int port) const;

};

#endif