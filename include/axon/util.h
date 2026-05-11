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
	struct timer {

		std::chrono::time_point<std::chrono::high_resolution_clock> _start, _end;
		std::vector<long> _laps;
		std::string _name;

		timer(const char *name):_name(name)
		{
			_start = std::chrono::high_resolution_clock::now();
		}

		timer(std::string &name):_name(name)
		{
			_start = std::chrono::high_resolution_clock::now();
		}

		~timer()
		{
			_end = std::chrono::high_resolution_clock::now();
			auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(_end - _start);
			INFPRN("%s ran for %ldμs", _name.c_str(), microseconds.count());
		}

		long now()
		{
			_end = std::chrono::high_resolution_clock::now();
			std::chrono::microseconds microseconds = std::chrono::duration_cast<std::chrono::microseconds>(_end - _start);
			return microseconds.count();
		}

		void reset()
		{
			_start = std::chrono::high_resolution_clock::now();
			_laps.clear();
		}

		void lap()
		{
			std::chrono::time_point<std::chrono::high_resolution_clock> _temp = std::chrono::high_resolution_clock::now();
			std::chrono::microseconds microseconds = std::chrono::duration_cast<std::chrono::microseconds>(_temp - _start);
			_laps.push_back(microseconds.count());
		}

		template <
			class result_t   = std::chrono::milliseconds,
			class clock_t    = std::chrono::steady_clock,
			class duration_t = std::chrono::milliseconds
		>
		auto since(std::chrono::time_point<clock_t, duration_t> const& start)
		{
			return std::chrono::duration_cast<result_t>(clock_t::now() - start);
		}

		static std::string iso8601()
		{
			std::stringstream ss;

			const auto current_time_point {std::chrono::system_clock::now()};
			const auto current_time_since_epoch {current_time_point.time_since_epoch()};
			const auto current_milliseconds {std::chrono::duration_cast<std::chrono::milliseconds> (current_time_since_epoch).count() % 1000};

			std::time_t t = std::chrono::system_clock::to_time_t(current_time_point);
			ss<<std::put_time(std::localtime(&t), "%FT%T")<<"."<<std::setw(3)<<std::setfill('0')<<current_milliseconds;

			return ss.str();
		}

		static long epoch()
		{
			return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}

		static long diff(const long ep)
		{
			return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() - ep;
		}

		template <
			class result_t   = std::chrono::milliseconds,
			class clock_t    = std::chrono::steady_clock,
			class duration_t = std::chrono::milliseconds
		>
		static auto diff(const std::chrono::time_point<std::chrono::high_resolution_clock> ep)
		{
			return std::chrono::duration_cast<result_t>(clock_t::now() - ep);
		}

		static std::string fulldate(std::time_t unixtime)
		{
			std::stringstream oss;
			std::tm* t = std::localtime(&unixtime);
			oss << std::put_time(t, "%Y-%m-%d %I:%M:%S %p");

			return oss.str();
		}

		static std::string fulldate(long long epoch_ms)
		{
			auto tp = std::chrono::system_clock::time_point{ std::chrono::milliseconds{epoch_ms} };

			// Convert to time_t for localtime (drops sub-second precision)
			std::time_t tt = std::chrono::system_clock::to_time_t(tp);

			// Extract millisecond remainder
			long long ms = epoch_ms % 1000;

			std::ostringstream oss;
			oss << std::put_time(std::localtime(&tt), "%Y-%m-%dT%H:%M:%S")
				<< '.'
				<< std::setfill('0') << std::setw(3) << ms
				<< 'Z';

			return oss.str();
		}
	};

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
