#include "../inc/ConfigValidator.hpp"
#include <sstream>
#include <cctype>
#include <iostream>
#include <unistd.h>

bool ConfigValidator::isInteger(const std::string& s)
{
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (!isdigit(s[i]))
            return false;
    }
    return true;
}

bool ConfigValidator::isValidPort(const std::string& s)
{
    if (!isInteger(s))
        return false;
    int port;

    port = atoi(s.c_str());
    if (port >= 0 && port <= 65535)
        return true;
    return false;
}

bool ConfigValidator::isValidIP(const std::string& ip)
{
    std::istringstream iss(ip);
    std::string token;
    int count = 0;

    while (std::getline(iss, token, '.'))
    {
        if (!isInteger(token))
            return false;
        int num;

        num = atoi(token.c_str());
        if (num < 0 || num > 255)
            return false;
        count++;
    }
    return count == 4;
}

bool ConfigValidator::isValidMethods(const std::string& val)
{
    std::istringstream iss(val);
    std::string method;
    
    while (iss >> method)
    {
        if (method != "GET" && method != "POST" && method != "DELETE")
            return false;
    }
    return true;
}

bool ConfigValidator::isValidErrorPage(const std::string& val)
{
    std::istringstream iss(val);
    std::string codeStr;
    std::string path;
    
    iss >> codeStr >> path;
    if (!isInteger(codeStr))
        return false;
    int code;
    
    code = atoi(codeStr.c_str());
    // TODO: Implement proper path validation later
    return (code >= 400 && code < 600) && !path.empty();  // Temporarily skip path existence check
    /*
    if ((code >= 400 && code < 600) && !path.empty() && access(path.c_str(), F_OK) == 0)
        return true;
    return false;
    */
}

bool ConfigValidator::isValidExtension(const std::string& s)
{
    const std::string ext[] = {".py", ".sh"};
    for (size_t i = 0; i < sizeof(ext); ++i)
    {
        if (s == ext[i])
            return true;
    }
    return false;
}

bool ConfigValidator::isValidPath(const std::string& Path)
{
    (void)Path;  // Mark parameter as unused
    // TODO: Implement proper path validation later
    return true;  // Temporarily accept all paths
    
    // Original validation commented out
    /*
    std::string actual = Path;
    if (!Path.empty() && Path[0] != '/') {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            return false;
        }
        actual = std::string(cwd) + "/" + Path;
    }
    return (access(actual.c_str(), F_OK) == 0);
    */
}

bool ConfigValidator::isValidIndex(const std::string& val)
{
    std::istringstream iss(val);
    std::string index;
    std::string path;

    while (iss >> index)
    {
        path = "/var/www/html/" + index;
        //a verifier
        // if (access(path.c_str(), F_OK) == 0)
            return true;
    }
    return false;
}

bool ConfigValidator::isValidName(const std::string& name)
{
    for(size_t i = 0; i < name.size() ; ++i)
    {
        char c = name[i];
        if(!isalnum(c) && c == '.')
            return false;
    }
    return true;
}

bool ConfigValidator::isValidAutoindex(const std::string& val)
{
    return (val == "on" || val == "off");
}

bool ConfigValidator::isValidBodySize(const std::string& val)
{
    return isInteger(val);
}

bool ConfigValidator::isValueValid(const std::string& key, const std::string& value)
{
    if (key == "listen")
        return isValidPort(value);

    if (key == "host")
        return isValidIP(value);

    if (key == "server_name")
        return isValidName(value);

    if (key == "autoindex" || key == "cgi")
        return (value == "on" || value == "off");

    if (key == "client_max_body_size")
        return isInteger(value);

    if (key == "allow_methods")
        return isValidMethods(value);

    if (key == "cgi_ext")
        return isValidExtension(value);

    if (key == "error_page")
        return isValidErrorPage(value);

    if (key == "index")
        return isValidIndex(value);

    if (key == "root" || key == "upload_dir" || key == "cgi_path")
        return isValidPath(value);

    return true;
}

bool ConfigValidator::validateServerConfig(const std::map<std::string, std::string>& config)
{
    (void)config;
    return true;
}

bool ConfigValidator::validateLocationConfig(const std::map<std::string, std::string>& config)
{
    (void)config;
    return true;
}

void ConfigValidator::printServers(const std::vector<Server>& servers)
{
    for (std::size_t s = 0; s < servers.size(); ++s)
    {
        std::cout << "\n--- SERVER " << s + 1 << " ---" << std::endl;

        std::map<std::string, std::string>::const_iterator it;
        for (it = servers[s].instruct.begin(); it != servers[s].instruct.end(); ++it)
        {
            std::cout << "  " << it->first << " : " << it->second << std::endl;
        }

        for (std::size_t i = 0; i < servers[s].locations.size(); ++i)
        {
            std::cout << "\n  Location: " << servers[s].locations[i].path << std::endl;

            std::map<std::string, std::string>::const_iterator lit;
            for (lit = servers[s].locations[i].instruct.begin();
                    lit != servers[s].locations[i].instruct.end(); ++lit)
            {
                std::cout << "    " << lit->first << " : " << lit->second << std::endl;
            }
        }
    }
} 