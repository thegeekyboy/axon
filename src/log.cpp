#include <iostream>
#include <cstring>

#include <axon.h>
#include <axon/log.h>
#include <axon/util.h>

namespace axon
{
	void log::fopen()
	{
		char filename[PATH_MAX];

		snprintf(filename, PATH_MAX - 1, "%s/%s", _path.c_str(), _filename.c_str());

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

		_level = level::info;
		_filename = "";

		char* cwd = get_current_dir_name();

		_path = cwd;
		free(cwd);

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

		snprintf(fullpath, PATH_MAX - 1, "%s/%s", _path.c_str(), _filename.c_str());

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
		auto [path, filename] = axon::util::splitpath(fname);

		if (path.size())_path = path;
		_filename = filename;

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

	int &log::operator[] ([[maybe_unused]]int i)
	{
		return _dummy;
	}

	log& log::operator<<(level value)
	{
		_level = value;

		return *this;
	}

	log& log::operator<<(bool value)
	{
		if (value)
			_ss<<"true";
		else
			_ss<<"false";

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

		std::stringstream oss;

		oss<<"["<<axon::timer::iso8601()<<" "<<std::setfill(' ')<<std::setw(7)<<getpid()<<" "<<std::this_thread::get_id()<<std::setw(9)<<l2n(_level)<<"] ";

		if (_writable)
			_ofs<<oss.rdbuf()<<_ss.rdbuf();
		else
			fprintf(stderr, "%s%s", oss.str().c_str(), _ss.str().c_str());

		_ss.str("");
		_ss.clear();

		return *this;
	}
}

