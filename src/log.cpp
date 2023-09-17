#include <axon.h>
#include <axon/log.h>

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
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot use " + _path + " for logging");
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

	void log::open(std::string filename)
	{
		size_t found;

		found = filename.find_last_of("/\\");
		_path = filename.substr(0, found);
		_filename = filename.substr(found + 1);

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

	log& log::operator<<(int iv)
	{
		printf(">>int>> %d\n", iv);

		return *this;
	}

	log& log::operator<<(std::string& sv)
	{
		printf(">>string>> %s\n", sv.c_str());

		return *this;
	}

	log& log::operator<<(const char *sv)
	{
		printf(">>char>> %s\n", sv);

		return *this;
	}

	log& log::operator<<([[maybe_unused]] std::ostream& (*fun)(std::ostream&))
	{
		std::cout<<std::endl;

		return *this;
	}
}