#include <axon.h>
#include <log.h>

#define CFG_LOG_PATH 'p'
#define CFG_LOG_FILENAME 'f'
#define CFG_LOG_LEVEL 0x0001

namespace axon
{
	log::log()
	{
		reset();
	}

	log::~log()
	{
		if (_ofs.is_open())
			_ofs.close();
	}

	int log::load(config_t *cfg)
	{
		const char *__path, *__filename;
		int retval = true;

		if (config_lookup_string(cfg, "log.path", &__path))
			_path = __path;
		else
		{
			std::cerr<<"Overmind - Mandatory configuration missing 'log.path'"<<std::endl;
			retval = false;
		}

		if (config_lookup_string(cfg, "log.filename", &__filename))
			_filename = __filename;
		else
		{
			std::cerr<<"Overmind - Mandatory configuration missing 'log.file'"<<std::endl;
			retval = false;
		}

		config_lookup_int(cfg, "log.level", &_level);

		return retval;
	}

	bool log::reset()
	{
		
		return true;
	}

	std::string log::operator[](char i)
	{
		if (i == CFG_LOG_PATH)
			return _path;
		else if (i == CFG_LOG_FILENAME)
			return _filename;

		return 0;
	}

	int log::operator[] (int i)
	{
		if (i == CFG_LOG_LEVEL)
			return _level;

		return 0;
	}

	bool log::open()
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

	bool log::close()
	{
		if (_ofs.is_open())
			_ofs.close();

		return true;
	}
}