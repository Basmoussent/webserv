#include "Webserv.hpp"

int main(int ac, char **av)
{
    if (ac < 2)
    {
        write(2, "Usage: ./webserv <config_file> or ./webserv <config_file> --test\n", 58);
        return 1;
    }

    ConfigParser parser;
    if (!parser.parseFile(av[1]))
    {
        write(2, "Configuration file parsing failed.\n", 34);
        return 1;
    }
    if (!parser.validateConfig())
    {
        write(2, "Configuration file validation failed.\n", 36);
        return 1;
    }

    runServer(parser);
    
    return 0;
}