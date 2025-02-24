#ifndef AXON_MASTER_H_
#define AXON_MASTER_H_

#include <string>
#include <utility>
#include <sstream>
#include <array>
#include <vector>
#include <functional>
#include <chrono>
#include <iomanip>

#include <sys/stat.h>

#define MAXBUF 2097152 //1048576

#ifndef PATH_MAX
	#define PATH_MAX 260
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define AXON_DEBUG 33
#define AXON_INFO 34
#define AXON_NOTICE 36
#define AXON_WARNING 35
#define AXON_ERROR 31

#ifndef DEBUG
	#define DEBUG 0
#endif

#if DEBUG == 1
	#define DBGPRN(...) RAWDBG(AXON_DEBUG, __VA_ARGS__)
	#define INFPRN(...) RAWDBG(AXON_INFO, __VA_ARGS__)
	#define NOTPRN(...) RAWDBG(AXON_NOTICE, __VA_ARGS__)
	#define WRNPRN(...) RAWDBG(AXON_WARNING, __VA_ARGS__)
	#define ERRPRN(...) RAWDBG(AXON_ERROR, __VA_ARGS__)
	extern void RAWDBG(int, const char*, ...);
	extern std::mutex printer_safety;
#else
	#define DBGPRN(...)
	#define INFPRN(...)
	#define NOTPRN(...)
	#define WRNPRN(...)
	#define ERRPRN(...)
#endif

// AXON Namespace
namespace axon
{
	typedef int flags_t;
	typedef int proto_t;
	typedef int auth_t;

	struct flags {

		static const flags_t UNKNOWN = -1;
		static const flags_t DIR = 0;
		static const flags_t FILE = 1;
		static const flags_t LINK = 2;
		static const flags_t CHAR = 3;
		static const flags_t BLOCK = 4;
		static const flags_t FIFO = 5;
		static const flags_t SOCKET = 6;
	};

	struct protocol {

		static const proto_t UNKNOWN = -1;
		static const proto_t NOTHING = 0; // wip
		static const proto_t FILE = 1; // done
		static const proto_t SFTP = 2; // done
		static const proto_t FTP = 3; // done
		static const proto_t S3 = 4; // done
		static const proto_t SAMBA = 5;
		static const proto_t HDFS = 6; // done
		static const proto_t AWS = 7; // done
		static const proto_t SCP = 8; // done
		static const proto_t DATABASE = 9;
		static const proto_t KAFKA = 10;
		static const proto_t HTTP = 11;
	};

	struct authtype {

		static const auth_t UNKNOWN = -1;
		static const auth_t PASSWORD = 0;
		static const auth_t PRIVATEKEY = 1;
		static const auth_t KERBEROS = 2;
		static const auth_t NTLM = 3;
	};

	struct entry {

		std::string name;
		int type;
		long long size;
		flags_t flag;
		proto_t et;
		struct stat st;
	};

	struct licensekey
	{
		char version[8];
		char key[64];
		char validity[64];
		int expiredate;
	};

	class exception : public std::exception {

		char _what[4096];
		std::string _msg;
		std::string message;

		std::string make_message(const char* filename, int linenum, const std::string& source, const std::string& message)
		{
			std::stringstream s;

			s<<filename<<":"<<linenum<<" => "<<source<<"(): "<<message<<std::endl;
			_msg = message;

			return s.str();
		};

		public:
		exception(): exception("none", 0, "axon::error", "no error message") { };
		exception(const std::string& message): exception("none", 0, "axon:error", std::move(message)) { };
		exception(const std::string& source, const std::string& message): exception("none", 0, std::move(source), std::move(message)) { };
		exception(const char* filename, int linenum, const std::string& source, const std::string& message): message(make_message(filename, linenum, source, message)) { };
		template<typename... Args> exception(const char* filename, int linenum, const std::string& source, const std::string& format, Args... args) {
			char output[4096];
			snprintf(output, 4096, format.c_str(), args...);
			message = make_message(filename, linenum, source, output);
		}

		~exception() throw() {};

		virtual const char* what() const throw () {

			return message.c_str();
		};

		virtual const char* msg() const throw () {

			return _msg.c_str();
		};
	};

	template<std::size_t N, class R, class...Args>
	struct lambda
	{
		static std::array< std::function<R(Args&&...)>, N >& table()
		{
			static std::array< std::function<R(Args&&...)>, N > arr;
			return arr;
		}

		template<std::size_t I>
		static R call( Args...args )
		{
			return table()[I]( std::forward<Args>(args)... );
		}

		using sig = R(Args...);

		template<std::size_t I=N-1>
		static sig* make(std::function<R(Args&&...)> f)
		{
			if(!table()[I])
			{
				table()[I]=f;
				return &call<I>;
			}

			if(I==0) return nullptr;

			return make< (I-1)%N >(f);
		}

		template<std::size_t I=N-1>
		static void recycle( sig* f )
		{
			if (f==call<I>)
			{
				table()[I]={};
				return;
			}

			if (I==0) return;

			recycle< (I-1)%N >(f);
		}
	};

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
	};
}

#endif
