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
		std::cerr << "Échec du parsing du fichier de configuration." << std::endl;
		return 1;
	}
	parser.printServers();
	return 0;
}