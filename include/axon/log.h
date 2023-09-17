#ifndef AXON_LOG_H_
#define AXON_LOG_H_

#include <fstream>
#include <mutex>
#include <thread>

#include <libconfig.h>
#include <boost/format.hpp>

#define AXON_LOG_PATH 'p'
#define AXON_LOG_FILENAME 'f'
#define AXON_LOG_LEVEL 0x0001

#define AXON_DEFAULT_FILENAME "axon.log"

namespace axon
{
	class log
	{
		std::string _filename, _path;
		int _level;
		bool _writable = false;
		FILE *_fd;
		std::ofstream _ofs;
		std::mutex _safety;

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

		log& operator<<(int);
		log& operator<<(std::string&);
		log& operator<<(const char *);
		log& operator<<(std::ostream& (*fun)(std::ostream&));

		template<typename... Arguments>
		void print(std::string level, std::string const& fmt, Arguments&&... args)
		{
			char text[4096];
			std::ostringstream oss;

			oss<<std::this_thread::get_id();

			time_t cur_time = time(NULL);
			struct tm *st_time = localtime(&cur_time);

			sprintf(text, "[%02d-%02d-%d %2.2d:%2.2d:%2.2d %6d %6s %8s] ", st_time->tm_mday, st_time->tm_mon+1, st_time->tm_year+1900, st_time->tm_hour, st_time->tm_min, st_time->tm_sec, getpid(), oss.str().c_str(), level.c_str());

			boost::format f(fmt);
			int unroll[] {0, (f % std::forward<Arguments>(args), 0)...};
			static_cast<void>(unroll);

			std::lock_guard<std::mutex> lock(_safety);

			if (_writable)
				_ofs<<text<<boost::str(f)<<std::endl;
			else
				std::cout<<text<<boost::str(f)<<std::endl;
		}
	};

}

#endif