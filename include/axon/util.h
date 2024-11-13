#ifndef AXON_UTIL_H_
#define AXON_UTIL_H_

#include <iostream>
#include <variant>
#include <cstdarg>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <curl/curl.h>

#include <axon.h>

namespace axon
{
	std::string version();

	namespace util
	{
		typedef unsigned char BYTE;

		char *trim(char *);
		unsigned long long bytestoull(const char *, const size_t);
		std::string bytestodecstring(const char *, const size_t);
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
		bool execmd(const char *cmd);
		std::string base64_encode(BYTE const* buf, unsigned int bufLen);
		std::string base64_encode(const std::string &);
		std::vector<BYTE> base64_decode(std::string const&);
		std::string uuid();
		double random(double, double);
		void debugprint(const char *, ...);
		std::string demangle(const char*);

		std::string protoname(axon::proto_t);
		axon::proto_t protoid(std::string&);
		std::string authname(axon::auth_t);
		axon::auth_t authid(std::string&);

		template <typename T>
		uint16_t count(std::vector<T> value)
		{
			uint16_t cnt = 0;

			for ([[maybe_unused]] T &elem : value)
				cnt++;

			return cnt;
		}

		template <typename T>
		uint16_t count(std::va_list list, const T first)
		{
			uint16_t cnt = 0;
			T bv = first;

			while (bv != nullptr)
			{
				cnt++;
				bv = va_arg(list, T);
			}

			return cnt;
		}

		template <typename T, typename... Args> struct concatenator;
		template <typename... Args0, typename... Args1>
		struct concatenator<std::variant<Args0...>, Args1...> {
			using type = std::variant<Args0..., Args1...>;
		};
	}
}

#endif
