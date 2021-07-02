#ifndef AXON_LOG_H_
#define AXON_LOG_H_

#include <libconfig.h>
#include <boost/format.hpp>

namespace axon
{
	class log
	{
		std::string _filename, _path;
		int _level;
		FILE *_fd;
		std::ofstream _ofs;
		std::mutex _mtx;

	public:
		log();
		~log();

		int load(config_t *);
		bool reset();
		
		bool open();
		bool close();

		std::string operator[] (char i);
		int operator[] (int i);

		template<typename... Arguments>
		bool print(std::string level, std::string const& fmt, Arguments&&... args)
		{
			char text[4096];
			time_t cur_time = time(NULL);
			struct tm *st_time = localtime(&cur_time);

			sprintf(text, "[%02d-%02d-%d %2.2d:%2.2d:%2.2d %6d %s] ", st_time->tm_mday, st_time->tm_mon+1, st_time->tm_year+1900, st_time->tm_hour, st_time->tm_min, st_time->tm_sec, getpid(), level.c_str());

			boost::format f(fmt);
			int unroll[] {0, (f % std::forward<Arguments>(args), 0)...};
			//static_cast<void>(unroll);

			std::lock_guard<std::mutex> lock(_mtx);
			_ofs<<text<<boost::str(f)<<std::endl;

			return true;
		};
	};

}

#endif