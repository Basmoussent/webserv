#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <cstdlib>
#include "ConfigTypes.hpp"

class ConfigParser
{
	private :
		int							_blockDepth;
		bool						_inServer;
		bool						_inLocation;
		bool						_bracket;
	
		std::map<std::string, int>	_keywords;
		std::vector<Server>			_servers;

		Server						_currentServer;
		Location					_currentLocation;

		bool		openFile(const std::string& filename, std::ifstream& file);
		bool		parseLine(const std::string& line);
		bool		handleServerBlock();
		bool		handleLocationBlock(std::istringstream& iss);
		bool		handleClosingBrace();
		bool		assignKeyValue(const std::string& key, std::istringstream& iss);
		
		bool		checkMinimumConfig() const;
		std::string	trim(const std::string& s);

		// Validation methods
		bool		isValidName(const std::string& name) const;
		bool		isInteger(const std::string& s) const;
		bool		isValueValid(const std::string& key, const std::string& value, const Location& loc, const Server& srv) const;
		bool		isValidPort(const std::string& s) const;
		bool		isValidIP(const std::string& ip) const;
		bool		isValidMethods(const std::string& val) const;
		bool		isValidExtension(const std::string& s) const;
		bool		isValidRelativPath(const std::string& path) const ;
		bool		isValidPath(const std::string& path, const Location& loc, const Server& srv, const std::string& key) const;
		bool		isValidCGIPath(const std::string& path) const;
		bool		isValidErrorPage(const std::string& val, const Location& loc, const Server& srv) const;
		bool		isValidIndex(const std::string& val, const Location& loc, const Server& srv) const;
		bool		isValidRedirect(const std::string& val) const;

	public :
		ConfigParser();
		~ConfigParser();
		
		bool			parseFile(const std::string& filename);
		bool			validateConfig() const;
		void			printServers() const;
		
		const			std::vector<Server>&	getServers() const;
		std::string		getInstruct(const std::string& key, const Server server) const;
		const Server&	getServerByPort(int port) const;
};

#endif