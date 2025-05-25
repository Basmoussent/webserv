#ifndef CONFIGVALIDATOR_HPP
#define CONFIGVALIDATOR_HPP

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include "ConfigTypes.hpp"

class ConfigValidator {
private:
    static bool isInteger(const std::string& str);
    static bool isValidPort(const std::string& port);
    static bool isValidIP(const std::string& ip);
    static bool isValidMethods(const std::string& methods);
    static bool isValidErrorPage(const std::string& errorPage);
    static bool isValidExtension(const std::string& extension);
    static bool isValidPath(const std::string& path);
    static bool isValidIndex(const std::string& index);
    static bool isValidName(const std::string& name);
    static bool isValidAutoindex(const std::string& autoindex);
    static bool isValidBodySize(const std::string& bodySize);

public:
    static bool isValueValid(const std::string& key, const std::string& value);
    static bool validateServerConfig(const std::map<std::string, std::string>& config);
    static bool validateLocationConfig(const std::map<std::string, std::string>& config);
    static void printServers(const std::vector<Server>& servers);
};

#endif 