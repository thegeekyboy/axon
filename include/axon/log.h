#ifndef AXON_LOG_H_
#define AXON_LOG_H_

#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>

#include <libconfig.h>
#include <boost/format.hpp>

#include <axon/util.h>

#define AXON_LOG_PATH 'p'
#define AXON_LOG_FILENAME 'f'
#define AXON_LOG_LEVEL 0x0001

#define AXON_DEFAULT_FILENAME "axon.log"
#define AXON_DEFAULT_FILENO stderr

namespace axon
{
	enum class level {emergency, alert, critical, error, warning, notice, info, debug};

	class log
	{
		std::string _pid;

		std::string _filename, _path;
		level _level;
		bool _writable = false;
		FILE *_fd { AXON_DEFAULT_FILENO };
		std::ofstream _ofs;
		std::mutex _safety;

		std::stringstream _ss;
		int _dummy;

		std::string l2n(axon::level l)
		{
			switch (l)
			{
				case level::emergency: return "EMERGENCY";
				case level::alert: return "    ALERT";
				case level::critical: return " CRITICAL";
				case level::error: return "    ERROR";
				case level::warning: return "  WARNING";
				case level::notice: return "   NOTICE";
				case level::info: return "     INFO";
				case level::debug: return "    DEBUG";
				default: return "     NULL";
			}
			return "     NULL";
		}
		void fopen();

	public:

		log();
		log(FILE*);
		~log();

		log(const log&);

		bool reset();
		void set(pid_t);

		void open();
		void open(FILE*);
		void open(std::string);
		void open(std::string, std::string);
		void close();

		std::string& operator[] (char i);
		int& operator[] (int i);

		log& operator<<(axon::level);
		log& operator<<(bool);
		log& operator<<(int);
		log& operator<<(long);
		log& operator<<(long long);
		log& operator<<(float);
		log& operator<<(double);
		log& operator<<(char);
		log& operator<<(const char *);
		log& operator<<(std::string&);
		log& operator<<(std::ostream& (*fun)(std::ostream&)); // this is for std::endl

		void print(axon::level, const std::string&);
		void print(std::string text) { print(axon::level::info, text); }

		template<typename... Arguments>
		void print(axon::level lvl, std::string const& fmt, Arguments&&... args)
		{
			boost::format f(fmt);
			int unroll[] {0, (f % std::forward<Arguments>(args), 0)...};
			static_cast<void>(unroll);

			print(lvl, boost::str(f));
		}
	};

}

#endif

