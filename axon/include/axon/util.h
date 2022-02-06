#ifndef AXON_UTIL_H_
#define AXON_UTIL_H_

namespace axon
{
	char *trim(char *);
	int mkdir(const std::string&, mode_t);
	std::tuple<std::string, std::string> splitpath(std::string);
	std::vector<std::string> split(const std::string&, const char);
	std::string hash(const std::string&);
	bool exists(const std::string&, const std::string&);
	bool execmd(const char *cmd, const char *name);
}

#endif