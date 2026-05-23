#ifndef AXON_MASTER_H_
#define AXON_MASTER_H_

#include <mutex>
#include <vector>
#include <string>
#include <cstring>
#include <sys/stat.h>

#ifndef PATH_MAX
	#define PATH_MAX 260
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define AXON_EMERG 35
#define AXON_ERROR 31
#define AXON_WARNING 33
#define AXON_DEBUG 36
#define AXON_INFO 34

#ifndef DEBUG
	#define DEBUG 0
#endif

#define MAXDBGLEN 4096

#define EMRPRN(...) axon::debug(stderr, __FILENAME__, __LINE__, __PRETTY_FUNCTION__, AXON_EMERG, __VA_ARGS__)
#define ERRPRN(...) axon::debug(stderr, __FILENAME__, __LINE__, __PRETTY_FUNCTION__, AXON_ERROR, __VA_ARGS__)

#if DEBUG >= 1
	#define DBGPRN(...) axon::debug(stderr, __FILENAME__, __LINE__, __PRETTY_FUNCTION__, AXON_DEBUG, __VA_ARGS__)
#else
	#define DBGPRN(...) { }
#endif

#if DEBUG >= 2
	#define WRNPRN(...) axon::debug(stderr, __FILENAME__, __LINE__, __PRETTY_FUNCTION__, AXON_WARNING, __VA_ARGS__)
#else
	#define WRNPRN(...) { }
#endif

#if DEBUG >= 3
	#define INFPRN(...) axon::debug(stderr, __FILENAME__, __LINE__, __PRETTY_FUNCTION__, AXON_INFO, __VA_ARGS__)
#else
	#define INFPRN(...) { }
#endif

// AXON Namespace
namespace axon
{
	constexpr std::size_t MAX_BUFFER_SIZE = 2097152;
	extern std::mutex spinlock;

	std::string version();

	enum class status: int8_t { unknown = 0, ready = 1, active = 2, disabled = 4 };

	class exception : public std::exception {

		char _what[4096];
		char _msg[3072];

		std::string make_message(const char* filename, int linenum, const std::string& source, const std::string& message)
		{
			snprintf(_what, 4095, "%s: %d => %s(): %s", filename, linenum, source.c_str(), message.c_str());
			strncpy(_msg, message.c_str(), 3071);

			return _what;
		};

		public:
		exception() = delete;
		exception(const std::string& message): exception("none", 0, "axon:error", std::move(message)) { };
		exception(const std::string& source, const std::string& message): exception("none", 0, std::move(source), std::move(message)) { };
		exception(const char* filename, int linenum, const std::string& source, const std::string& message) { make_message(filename, linenum, source, message); };
		template<typename... Args> exception(const char* filename, int linenum, const std::string& source, const std::string& format, Args... args) {
			char output[4096];
			snprintf(output, 4095, format.c_str(), args...);
			make_message(filename, linenum, source, output);
		}

		~exception() throw() {};

		virtual const char* what() const throw () {

			return _what;
		};

		virtual const char* msg() const throw () {

			return _msg;
		};
	};

	template<typename T, typename = void>
	struct has_c_str : std::false_type {};

	template<typename T>
	struct has_c_str<T, std::void_t<decltype(std::declval<T>().c_str())>> : std::true_type {};

	template<typename T>
	decltype(auto) printable(T&& t)
	{
		if constexpr (has_c_str<std::decay_t<T>>::value)
			return t.c_str();
		else
			return std::forward<T>(t);
	}

	template<typename... Args>
	void debug(FILE* fp, const char* filename, int line, const char* func, int code, const char* format, Args&&... args)
	{
		std::lock_guard<std::mutex> lock(axon::spinlock);
		char refmt[MAXDBGLEN], buf[32], out[40];
		struct tm tm_info;

		const auto now = std::chrono::system_clock::now();
		const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
		const std::time_t t = std::chrono::system_clock::to_time_t(now);

		localtime_r(&t, &tm_info);

		strftime(buf, sizeof(buf), "%FT%T", &tm_info);
		snprintf(out, sizeof(out), "%s.%03ld", buf, ms);

		snprintf(refmt, MAXDBGLEN - 1, "\033[0;%dm[%s] %s(%d) %s: %s\033[0m\n", code, out, filename, line, func, format);
		fprintf(fp, refmt, printable(std::forward<Args>(args))...);
		fflush(fp);
	}

	struct timer {

		std::chrono::time_point<std::chrono::high_resolution_clock> _start, _end;
		std::vector<int64_t> _laps;
		std::string _name;

		timer(const char *name): _name(name)
		{
			_start = std::chrono::high_resolution_clock::now();
		}

		timer(const std::string &name): _name(name)
		{
			_start = std::chrono::high_resolution_clock::now();
		}

		~timer()
		{
			auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - _start);
			INFPRN("%s ran for %ldμs", _name.c_str(), microseconds.count());
		}

		long now() const
		{
			std::chrono::microseconds microseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - _start);
			return microseconds.count();
		}

		void reset()
		{
			_start = std::chrono::high_resolution_clock::now();
			_laps.clear();
		}

		void lap()
		{
			auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - _start);
			_laps.push_back(elapsed.count());
		}

		template <
			class result_t   = std::chrono::milliseconds,
			class clock_t    = std::chrono::steady_clock,
			class duration_t = std::chrono::milliseconds
		>
		static auto since(std::chrono::time_point<clock_t, duration_t> const& start)
		{
			return std::chrono::duration_cast<result_t>(clock_t::now() - start);
		}

		static std::string iso8601()
		{
			const auto now       = std::chrono::system_clock::now();
			const auto ms        = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
			const std::time_t t  = std::chrono::system_clock::to_time_t(now);

			struct tm tm_info;
			localtime_r(&t, &tm_info);

			char buf[32], out[40];

			strftime(buf, sizeof(buf), "%FT%T", &tm_info);
			snprintf(out, sizeof(out), "%s.%03ld", buf, ms);

			return out;
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
			class clock_t	= std::chrono::steady_clock,
			class duration_t = std::chrono::milliseconds
		>
		static auto diff(const std::chrono::time_point<std::chrono::high_resolution_clock> ep)
		{
			return std::chrono::duration_cast<result_t>(clock_t::now() - ep);
		}

		static std::string fulldate(std::time_t unixtime)
		{
			struct tm tm_info;
			localtime_r(&unixtime, &tm_info);

			char buf[32];
			strftime(buf, sizeof(buf), "%Y-%m-%d %I:%M:%S %p", &tm_info);
			return buf;
		}

		static std::string fulldate(long long epoch_ms)
		{
			auto tp = std::chrono::system_clock::time_point{std::chrono::milliseconds{epoch_ms}};
			std::time_t tt = std::chrono::system_clock::to_time_t(tp);
			long long ms   = epoch_ms % 1000;

			struct tm tm_info;
			localtime_r(&tt, &tm_info);

			char buf[32];
			strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm_info);

			char out[40];
			snprintf(out, sizeof(out), "%s.%03lldZ", buf, ms);

			return out;
		}
	};
}

#endif