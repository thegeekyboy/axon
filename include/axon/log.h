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

namespace axon
{
	enum class level {emergency, alert, critical, error, warning, notice, info, debug};

	class log
	{
		std::string _filename, _path;
		level _level;
		bool _writable = false;
		FILE *_fd;
		std::ofstream _ofs;
		std::mutex _safety;

		std::stringstream _ss;
		int _dummy;

		std::string l2n(axon::level l) {
			if (l == level::emergency) return "EMERGENCY";
			else if (l == level::alert) return "ALERT";
			else if (l == level::critical) return "CRITICAL";
			else if (l == level::error) return "ERROR";
			else if (l == level::warning) return "WARNING";
			else if (l == level::notice) return "NOTICE";
			else if (l == level::info) return "INFO";
			else if (l == level::debug) return "DEBUG";
			return "NULL";
		}
		void fopen();

	public:

		log();
		~log();

		log(const log&);

		void open();
		void open(std::string);
		void open(std::string, std::string);
		void close();
		bool reset();

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

		template<typename... Arguments>
		void print(axon::level lvl, std::string const& fmt, Arguments&&... args)
		{
			std::stringstream oss;

			oss<<"["<<axon::timer::iso8601()<<" "<<std::setfill(' ')<<std::setw(7)<<getpid()<<" "<<std::this_thread::get_id()<<" "<<std::setw(9)<<l2n(lvl)<<"] ";

			boost::format f(fmt);
			int unroll[] {0, (f % std::forward<Arguments>(args), 0)...};
			static_cast<void>(unroll);

			std::lock_guard<std::mutex> lock(_safety);

			if (_writable)
				_ofs<<oss.rdbuf()<<boost::str(f)<<std::endl;
			else
				fprintf(stderr, "%s%s\n", oss.str().c_str(), boost::str(f).c_str());
		}

		void print(std::string text) { print(axon::level::info, text); }
	};

}

#endif
