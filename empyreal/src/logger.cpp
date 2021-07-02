#include <main.h>

logger::logger()
{
    mu = new std::mutex;
}

logger::logger(std::mutex *mtxlg, std::string filename, std::string path, int level)
{
    mu = mtxlg;
	reset();
    _filename = filename;
    _path = path;
    _level = level;
}

logger::~logger()
{
	if (_ofs.is_open())
		_ofs.close();

    // delete mu; how to delete this?
}

int logger::reset()
{
	_level = 0;
	_filename = "";
	_path = "";

	if (_ofs.is_open())
		_ofs.close();

	return true;
}

std::string logger::operator[](char i)
{
	if (i == CFG_LOG_PATH)
		return _path;
	else if (i == CFG_LOG_FILENAME)
		return _filename;

	return 0;
}

int logger::operator[] (int i)
{
	if (i == CFG_LOG_LEVEL)
		return _level;

	return 0;
}

int logger::open(std::string path, std::string filename)
{
    reset();

    _path = path;
    _filename = filename;

	return true;	
}

int logger::open()
{
	char destination[512];

	sprintf(destination, "%s/%s", _path.c_str(), _filename.c_str());

	_ofs.open(destination, std::fstream::app);

	if (!_ofs.is_open())
	{
		//fprintf(stderr, "Error opening log file %s. Cannot output log (%s), aborting process...\n", destination, strerror (errno));
		return false;
	}

	return true;
}

int logger::close()
{
	if (_ofs.is_open())
		_ofs.close();
}