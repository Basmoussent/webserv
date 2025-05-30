#include "../includes/ConfigParser.hpp"

bool ConfigParser::validateConfig() const
{
	for (size_t s = 0; s < _servers.size(); ++s)
	{
		const Server& srv = _servers[s];
		Location emptyLoc;

		for (std::map<std::string, std::string>::const_iterator it = srv.instruct.begin();
			 it != srv.instruct.end(); ++it)
		{
			if (!isValueValid(it->first, it->second, emptyLoc, srv))
			{
				std::cerr << "Error: invalid value for '" << it->first << "' in server block." << std::endl;
				return false;
			}
		}
		for (size_t l = 0; l < srv.locations.size(); ++l)
		{
			const Location& loc = srv.locations[l];
			for (std::map<std::string, std::string>::const_iterator lit = loc.instruct.begin();
				 lit != loc.instruct.end(); ++lit)
			{
				if (!isValueValid(lit->first, lit->second, loc, srv))
				{
					std::cerr << "Error: invalid value for '" << lit->first << "' in location '" << loc.path << "'." << std::endl;
					return false;
				}
			}
		}
	}
	return true;
}

bool ConfigParser::isValueValid(const std::string& key, const std::string& value, const Location& loc, const Server& srv) const
{
	if (key == "listen")
		return isValidPort(value);

	if (key == "host")
		return isValidIP(value);
		
	if (key == "server_name")
		return isValidName(value);
		
	if (key == "client_max_body_size")
			return isInteger(value);

	if (key == "autoindex" || key == "cgi")
		return (value == "on" || value == "off");		
		
	if (key == "allow_methods")
		return isValidMethods(value);
		
	if (key == "cgi_ext")
		return isValidExtension(value);
	
	if (key == "root")
		return isValidRoot(value);

	if (key == "error_page")
		return isValidErrorPage(value, loc, srv);
		
	if (key == "index")
		return isValidIndex(value, loc, srv);

	if (key == "cgi_path" || key == "upload_dir")
		return isValidPath(value, loc, srv, "root");

	if (key == "return")
		return isValidRedirect(value, loc, srv);
	
	return true;
}

bool	ConfigParser::isValidName(const std::string& val) const
{
	std::istringstream iss(val);
	std::string tmp;

	while(iss >> tmp)
	{
		for(size_t i = 0; i < tmp.size() ; ++i)
		{
			char c = tmp[i];
			if(!isalnum(c) && c != '.')
				return false;
		}
	}
	return true;
}

bool ConfigParser::isValidExtension(const std::string& s) const
{
	const std::string ext[] = {".py", ".sh"};
	for (size_t i = 0; i < sizeof(ext); ++i)
	{
		if (s == ext[i])
			return true;
	}
	return false;

}

bool ConfigParser::isInteger(const std::string& s) const
{
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (!isdigit(s[i]))
			return false;
	}
	return true;
}

bool ConfigParser::isValidPort(const std::string& s) const
{
	if (!isInteger(s))
		return false;
	int port;

	port = atoi(s.c_str());
	if (port >= 0 && port <= 65535)
		return true;
	return false;
}

bool ConfigParser::isValidIP(const std::string& ip) const
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

bool ConfigParser::isValidMethods(const std::string& val) const
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

bool ConfigParser::isValidRoot(const std::string& path) const
{
	if (access(path.c_str(), F_OK) == 0)
		return true;
	return false;
}

bool	ConfigParser::isValidPath(const std::string& path, const Location& loc, const Server& srv, const std::string& key) const
{
	if (path[0] == '.')
		return (access(path.c_str(), F_OK) == 0);

	std::map<std::string, std::string>::const_iterator it = loc.instruct.find(key);
	std::string root;
	if (it != loc.instruct.end())
		root = it->second;
	it = srv.instruct.find(key);
	if (it != srv.instruct.end() && root.empty())
		root = it->second;

	std::string fullPath = root;
	if (!fullPath.empty() && fullPath[fullPath.size() - 1] != '/')
		fullPath += "/";
	fullPath += path;
	if (access(fullPath.c_str(), F_OK) == 0)
		return true;
	return false;
}

bool ConfigParser::isValidErrorPage(const std::string& val, const Location& loc, const Server& srv) const
{
	std::istringstream iss(val);
	std::string code;
	std::string path;

	iss >> code >> path;
	if (!isInteger(code))
		return false;
	int nb = atoi(code.c_str());
	if (nb < 400 && nb >= 600)
		return false;
	return (isValidPath(path, loc, srv, "root"));
}

bool	ConfigParser::isValidIndex(const std::string& val, const Location& loc, const Server& srv) const
{
	std::istringstream iss(val);
	std::string path;

	while (iss >> path) 
	{
		if (isValidPath(path, loc, srv, "root"))
			return true;
	}
	return false;
}

//je gerer pas tout encore car j'ai pas tout capter mdr
bool	ConfigParser::isValidRedirect(const std::string& val, const Location& loc, const Server& srv) const
{
	std::istringstream iss(val);
	std::string codeStr;
	std::string path;

	iss >> codeStr >> path;
		return false;
	int code = atoi(codeStr.c_str());
	std::cout << "code : " << code << std::endl;
	if (code != 301 && code != 302 && code != 307 && code != 308)
		return false;
	if (path.find("http://") == 0 || path.find("https://") == 0)
		return true;
	return (isValidPath(path, loc, srv, "root"));
}
