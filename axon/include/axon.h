#ifndef AXON_MASTER_H_
#define AXON_MASTER_H_

#include <iostream>
#include <sstream>
#include <array>
#include <functional>
#include <chrono>
// #include <fstream>
// #include <string>
// #include <cstring>
// #include <thread>
// #include <mutex>
// #include <cstdarg>
// #include <exception>
// #include <typeinfo>
// #include <iomanip>
// #include <atomic>
// #include <stack>
// #include <queue>

#include <string.h>
#include <linux/limits.h>

#include <sys/stat.h>
// #include <sys/types.h>
// #include <unistd.h>

#define MAXBUF 1048576

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#ifndef DEBUG
#define DEBUG 0
#endif
#if DEBUG == 1
#define DBGPRN(...) fprintf(stderr, __VA_ARGS__); puts("")
#else
#define DBGPRN(...)
#endif

// AXON Namespace
namespace axon
{
	typedef unsigned int flags_t;
	typedef unsigned int proto_t;
	typedef unsigned int auth_t;
	typedef unsigned int trans_t;

	struct flags {

		static const flags_t DIR = 1;
		static const flags_t FILE = 2;
		static const flags_t LINK = 4;
		static const flags_t CHAR = 8;
		static const flags_t BLOCK = 16;
		static const flags_t FIFO = 32;
		static const flags_t SOCKET = 64;
		static const flags_t UNKNOWN = 64;
	};

	struct protocol {

		static const proto_t FILE = 0; // done
		static const proto_t SFTP = 1; // done
		static const proto_t FTP = 2; // done
		static const proto_t S3 = 3; // done
		static const proto_t SAMBA = 4;
		static const proto_t SCP = 5;
		static const proto_t AWS = 6;
		static const proto_t HDFS = 7;
		static const proto_t DATABASE = 8;
		static const proto_t KAFKA = 9;
	};

	struct authtypes {

		static const auth_t PASSWORD = 0;
		static const auth_t PRIVATEKEY = 1;
		static const auth_t KERBEROS = 2;
		static const auth_t ADC = 3;
	};

	struct transaction {

		static const trans_t END = 0;
		static const trans_t BEGIN = 1;
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
			DBGPRN("%s ran for %ldμs\n", _name.c_str(), microseconds.count());
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
			return std::chrono::duration_cast<result_t>(clock_t::now() - _start);
		}

		//time since epoch = std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()).count()
	};
}

#endif
