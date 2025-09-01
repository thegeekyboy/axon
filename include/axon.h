#ifndef AXON_MASTER_H_
#define AXON_MASTER_H_

#include <string>
#include <cstring>
#include <sys/stat.h>

#define MAXBUF 2097152

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

#include <mutex>
#define MAXDBGLEN 4096

#define EMRPRN(...) axon::debug(stderr, __FILENAME__, __LINE__, __PRETTY_FUNCTION__, AXON_EMERG, __VA_ARGS__)
#define ERRPRN(...) axon::debug(stderr, __FILENAME__, __LINE__, __PRETTY_FUNCTION__, AXON_ERROR, __VA_ARGS__)

#ifdef DEBUG
	#define DBGPRN(...) axon::debug(stderr, __FILENAME__, __LINE__, __PRETTY_FUNCTION__, AXON_DEBUG, __VA_ARGS__)
#else
	#define DBGPRN(...) { }
#endif

#if DEBUG >= 1
	#define WRNPRN(...) axon::debug(stderr, __FILENAME__, __LINE__, __PRETTY_FUNCTION__, AXON_WARNING, __VA_ARGS__)
#else
	#define WRNPRN(...) { }
#endif

#if DEBUG >= 2
	#define INFPRN(...) axon::debug(stderr, __FILENAME__, __LINE__, __PRETTY_FUNCTION__, AXON_INFO, __VA_ARGS__)
#else
	#define INFPRN(...) { }
#endif

// AXON Namespace
namespace axon
{
	typedef int flags_t;
	typedef int proto_t;
	typedef int auth_t;

	extern void debug(FILE *, std::string, int, std::string, int, const char *, ...);
	extern std::mutex spinlock;

	std::string version();

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
		static const proto_t NOTHING = 0; // done
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
			snprintf(output, 4096, format.c_str(), args...);
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
}

#endif
