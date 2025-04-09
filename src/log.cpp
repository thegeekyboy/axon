#include <cstring>

#include <axon.h>
#include <axon/log.h>
#include <axon/util.h>

namespace axon
{
	void log::fopen()
	{
		char filename[PATH_MAX];

		sprintf(filename, "%s/%s", _path.c_str(), _filename.c_str());

		_ofs.open(filename, std::ofstream::out | std::ofstream::app);

		if (!_ofs.is_open() || _ofs.fail() == 1)
		{
			char errmsg[5120];

			sprintf(errmsg, "Error opening log file %s. Cannot output log (%s), aborting process...", filename, strerror (errno));
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, errmsg);
		}

		_writable = true;
	}

	log::log()
	{
		reset();
	}

	log::~log()
	{
		if (_ofs.is_open())
			_ofs.close();
	}

	log::log(const log& src)
	{
		_level = src._level;
		_filename = src._filename;
		_path = src._path;

		open();
	}

	bool log::reset()
	{
		if (_ofs.is_open())
			_ofs.close();

		_level = 0;
		_filename = "";
		_path = "";

		return true;
	}

	void log::open()
	{
		struct stat s;
		int code;

		if (_ofs.is_open())
			return;

		if ((_path.size() + _filename.size()) < 2)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "logging destination is invalid or empty!");


		if (stat(_path.c_str(), &s) == 0)
		{
			if (!(s.st_mode & S_IFDIR))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _path + " is a not directory");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot use " + _path + " for logging");

		char fullpath[PATH_MAX];

		sprintf(fullpath, "%s/%s", _path.c_str(), _filename.c_str());

		if ((code = stat(fullpath, &s)) == 0)
		{
			if (!(s.st_mode & S_IFREG))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot use " + _filename + " for logging, its a directory");
		}
		else if (errno != ENOENT)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot use " + _filename + " for logging, code: " + std::to_string(errno));

		fopen();
	}

	void log::open(std::string fname)
	{
		// size_t found;

		auto [path, filename] = axon::util::splitpath(fname);
		_path = path;
		_filename = filename;
		// found = filename.find_last_of("/\\");
		// _path = filename.substr(0, found);
		// _filename = filename.substr(found + 1);

		open();
	}

	void log::open(std::string path, std::string filename)
	{
		_path = path;
		_filename = filename;

		open();
	}

	void log::close()
	{
		if (_ofs.is_open())
			_ofs.close();
	}

	std::string &log::operator[](char i)
	{
		if (i == AXON_LOG_PATH)
			return _path;
		else if (i == AXON_LOG_FILENAME)
			return _filename;

		return _path;
	}

	int &log::operator[] (int i)
	{
		if (i == AXON_LOG_LEVEL)
			return _level;

		return _level;
	}

	log& log::operator<<(bool value)
	{
		_ss<<value;

		return *this;
	}

	log& log::operator<<(int value)
	{
		_ss<<value;

		return *this;
	}

	log& log::operator<<(long value)
	{
		_ss<<value;

		return *this;
	}

	log& log::operator<<(long long value)
	{
		_ss<<value;

		return *this;
	}

	log& log::operator<<(float value)
	{
		_ss<<value;

		return *this;
	}

	log& log::operator<<(double value)
	{
		_ss<<value;

		return *this;
	}

	log& log::operator<<(const char *value)
	{
		_ss<<value;

		return *this;
	}

	log& log::operator<<(std::string& value)
	{
		_ss<<value;

		return *this;
	}

	log& log::operator<<([[maybe_unused]] std::ostream& (*fun)(std::ostream&)) // this is for std::endl
	{
		std::lock_guard<std::mutex> lock(_safety);

		_ss<<std::endl;

		char text[512];
		time_t cur_time = time(NULL);
		struct tm *st_time = localtime(&cur_time);

		sprintf(text, "[%02d-%02d-%d %2.2d:%2.2d:%2.2d %6d] ", st_time->tm_mday, st_time->tm_mon+1, st_time->tm_year+1900, st_time->tm_hour, st_time->tm_min, st_time->tm_sec, getpid());

		if (_writable)
			_ofs<<"["<<axon::timer::iso8601()<<"] "<<_ss.rdbuf();
		else
			std::cout<<text<<_ss.rdbuf();

		return *this;
	}
}
