#ifndef CONFIGTYPES_HPP
#define CONFIGTYPES_HPP

#include <string>
#include <map>
#include <vector>

struct Location {
    std::string path;
    std::map<std::string, std::string> instruct;
};

struct Server {
    std::map<std::string, std::string> instruct;
    std::vector<Location> locations;
};

#endif 