#ifndef AXON_UTIL_H_
#define AXON_UTIL_H_

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp> 

#include <curl/curl.h>

namespace axon
{
	namespace helper
	{
		typedef unsigned char BYTE;
		
		char *trim(char *);
		// int mkdir(const std::string&, mode_t);
		std::tuple<std::string, std::string> splitpath(std::string);
		std::vector<std::string> split(const std::string&, const char);
		std::string hash(const std::string&);
		std::string md5(std::string&);
		bool makedir(const char *);
		bool isdir(const std::string&);
		bool isfile(const std::string&);
		bool exists(const std::string&);
		bool exists(const std::string&, const std::string&);
		bool iswritable(const std::string&);
		std::tuple<std::string, std::string> magic(std::string&);
		bool execmd(const char *cmd, const char *name);
		std::string base64_encode(BYTE const* buf, unsigned int bufLen);
		std::string base64_encode(const std::string &);
		std::vector<BYTE> base64_decode(std::string const&);
		std::string uuid();
		std::string demangle(const char*);

		template <typename T>
		uint16_t count(std::vector<T> value)
		{
			uint16_t cnt = 0;

			for (T &elem : value)
				cnt++;

			return cnt;
		}
	}
}

#endif