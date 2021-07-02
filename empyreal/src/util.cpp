#include <main.h>

char *trim(char *str)
{
	char *end;

	while(((unsigned char)*str) == ' ')
		str++;

	if(*str == 0)
		return str;

	end = str + strlen(str) - 1;
	
	while(end > str && isspace((unsigned char)*end))
		end--;

	*(end+1) = 0;

	return str;
}

int _mkdir(const char *path, mode_t mode)
{
	char tmp[PATH_MAX];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp),"%s", path);
	len = strlen(tmp);

	if(tmp[len - 1] == '/')
		tmp[len - 1] = 0;

	for(p = tmp + 1; *p; p++)
	{
		if(*p == '/') 
		{
			*p = 0;
			mkdir(tmp, mode);
			*p = '/';
		}
	}

	return mkdir(tmp, mode);
}

std::vector<std::string> split(const std::string& str, const char delim)
{
	std::vector<std::string> tokens;
	size_t prev = 0, pos = 0;

	do {

		pos = str.find(delim, prev);

		if (pos == std::string::npos)
			pos = str.length();

		std::string token = str.substr(prev, pos - prev);

		if (!token.empty())
			tokens.push_back(token);

		prev = pos + 1;

	} while (pos < str.length() && prev < str.length());

	return tokens;
}
