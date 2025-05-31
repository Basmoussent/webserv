#ifndef CONFIGTYPES_HPP
# define CONFIGTYPES_HPP

#include <string>
#include <map>
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

#endif 