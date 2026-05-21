#ifndef AXON_UTIL_H_
#define AXON_UTIL_H_

#include <vector>
#include <cstdarg>
#include <chrono>
#include <iomanip>
#include <mutex>

#include <cstdint>
#include <string_view>
#include <charconv>
#include <stdexcept>
#include <system_error>
#include <type_traits>

#include <axon.h>

namespace axon
{
	namespace util
	{
		typedef unsigned char BYTE;

		unsigned long long bytes_to_ull(const char*, size_t);

		unsigned long long bytestoull(const char *, const size_t);
		std::string bytestodecstring(const char *, const size_t);

		char *trim(char *);

		std::vector<std::string> split(const std::string&, const char);
		std::string merge(std::vector<std::string>, const char);

		std::tuple<std::string, std::string> splitpath(std::string);
		std::tuple<std::string, std::string> splitbucket(std::string);

		std::string hash(const std::string&);
		std::string md5(std::string&);

		// int mkdir(const std::string&, mode_t);
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

		bool set_thread_name(pthread_t, std::string);
		void rm_thread(std::vector<std::thread>&, std::thread::id);

		void debugprint(const char *, ...);
		std::string demangle(const char*);

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

		template <typename T>
		inline T str_to_num(std::string_view sv)
		{
			static_assert(std::is_integral_v<T>, "T must be an integral type");

			T result{};
			auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);

			if (ec == std::errc::invalid_argument)
				throw std::invalid_argument(std::string("non-numeric input '") + std::string(sv) + "'");

			if (ec == std::errc::result_out_of_range)
				throw std::overflow_error(std::string("value '") + std::string(sv) + "' out of range for type (max=" + std::to_string(std::numeric_limits<T>::max()) + ")");

			if (ptr != sv.data() + sv.size())
				throw std::invalid_argument(std::string("trailing non-numeric characters in '") + std::string(sv) + "'");

			return result;
		}
	}
}

#endif
