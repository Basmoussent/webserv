#include "./includes/ConfigParser.hpp"

int main(int ac, char **av)
{
	if (ac < 2)
	{
		std::cerr << "Usage: ./a.out <config_file>" << std::endl;
		return 1;
	}

	ConfigParser parser;
	if (!parser.parseFile(av[1]))
	{
		std::cerr << "Configuration file parsing failed." << std::endl;
		return 1;
	}
	 if (!parser.validateConfig())
	{
		std::cerr << "Configuration file validation failed." << std::endl;
		return 1;
	}
	parser.printServers();
	std::cout << "N'oublie pas que tu as des reves a accomplir !" << std::endl;
	return 0;
}