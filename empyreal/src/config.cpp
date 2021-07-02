#include <main.h>

config::config()
{

}

config::config(std::string filename)
{
	load(filename);
}

config::~config()
{
	config_destroy(&_cfg);
	std::cerr<<"Overmind - destroying configuration data"<<std::endl;
}

int config::reload()
{
	std::cout<<"Reloading configuration file..."<<std::endl;

	_version = "";
	_key = "";
	_validity = "";
	_basedir = "";
	_mode = 0;

	load(_filename);
}

int config::load(std::string filename)
{
	int retval = true;
	const char *buffer;

	config_init(&_cfg);

	_filename = filename;

	if(!config_read_file(&_cfg, filename.c_str()))
	{
		std::ostringstream _tmp;

		_tmp<<"Error reading config file "<<config_error_file(&_cfg)<<" at line "<<config_error_line(&_cfg)<<" - "<<config_error_text(&_cfg);
		_errstr = _tmp.str();
		_errno = CFG_E_FILEPARSE;

		config_destroy(&_cfg);

		return false;
	}

	if(config_lookup_string(&_cfg, "version", &buffer))
		_version = buffer;
	
	if(config_lookup_string(&_cfg, "key", &buffer))
		_key = buffer;
	else
	{
		std::cerr<<"Overmind - Mandatory configuration missing 'master.key'"<<std::endl;
		retval = false;
	}

	if(config_lookup_string(&_cfg, "validity", &buffer))
		_validity = buffer;
	else
	{
		std::cerr<<"Overmind - Mandatory configuration missing 'master.validity'"<<std::endl;
		retval = false;
	}
	
	if(config_lookup_string(&_cfg, "base", &buffer))
		_basedir = buffer;
	else
	{
		std::cerr<<"Overmind - Mandatory configuration missing 'master.base'"<<std::endl;
		retval = false;
	}
	
	if(!config_lookup_int(&_cfg, "mode", &_mode))
	{
		std::cerr<<"Overmind - Mandatory configuration missing 'master.mode'"<<std::endl;
		retval = false;
	}

	if (config_lookup_string(&_cfg, "log.path", &buffer))
		_log_path = buffer;
	else
	{
		std::cerr<<"Overmind - Mandatory configuration missing 'log.path'"<<std::endl;
		retval = false;
	}

	if (config_lookup_string(&_cfg, "log.filename", &buffer))
		_log_filename = buffer;
	else
	{
		std::cerr<<"Overmind - Mandatory configuration missing 'log.file'"<<std::endl;
		retval = false;
	}

	config_lookup_int(&_cfg, "log.level", &_log_level);

	return retval;
}

std::string config::errstr()
{
	return _errstr;
}

std::string config::operator[](char i)
{
	if (i == CFG_FILE_PATH)
		return _path;
	else if (i == CFG_FILE_NAME)
		return _filename;
	else if ( i == CFG_LOG_PATH)
		return _log_path;
	else if ( i == CFG_LOG_FILENAME)
		return _log_filename;

	//throw some error here
}

int config::operator[](int i)
{
	if (i == CFG_LOG_LEVEL)
		return _log_level;

	//throw some error here
}

config_setting_t* config::settings()
{
	return _settings;
}

// bool config::populate(logger *lgx)
// {
// 	lgx->reset();
// 	if (!lgx->load(&_cfg))
// 		return false;

// 	lgx->open();
// }

// bool config::populate(tcn::database::sqlite *dbp)
// {
// 	dbp->reset();
// 	if (!dbp->load(&_cfg))
// 		return false;

// 	return true;
// }

// bool config::populate(nodes *rt)
// {
// 	rt->reset();
// 	if (!rt->load(_cfg))
// 		return false;

// 	return true;
// }

// std::string config::path()
// {
// 	return _filename;
// }


// int config::errno()
// {
// 	return _errno;
// }

// END