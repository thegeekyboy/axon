#ifndef AXON_LOG_H_
#define AXON_LOG_H_

#include <fstream>
#include <sstream>
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

		std::stringstream _ss;

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
		void print(std::string, std::string const&, Arguments&&...);
	};

}

#endif
