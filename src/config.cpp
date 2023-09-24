#include <axon.h>
#include <axon/config.h>

#include <boost/filesystem.hpp>

/*
	Setting copy functions are copied from https://github.com/hyperrealm/libconfig/blob/master/contrib/copy_setting.c

	Thanks to who ever contributed it.
*/

void config_setting_copy_simple(config_setting_t * parent, const config_setting_t * src);
void config_setting_copy_elem(config_setting_t * parent, const config_setting_t * src);

void config_setting_copy_aggregate(config_setting_t * parent, const config_setting_t * src);
int config_setting_copy(config_setting_t * parent, const config_setting_t * src);

void config_setting_copy_simple(config_setting_t * parent, const config_setting_t * src)
{
	if(config_setting_is_aggregate(src))
	{
		config_setting_copy_aggregate(parent, src);
	}
	else 
	{
		config_setting_t * set;
		
		set = config_setting_add(parent, config_setting_name(src), config_setting_type(src));

		if(CONFIG_TYPE_INT == config_setting_type(src))
		{
			config_setting_set_int(set, config_setting_get_int(src));
			config_setting_set_format(set, src->format);
		}
		else if(CONFIG_TYPE_INT64 == config_setting_type(src))
		{
			config_setting_set_int64(set, config_setting_get_int64(src));
			config_setting_set_format(set, src->format);
		}
		else if(CONFIG_TYPE_FLOAT == config_setting_type(src))
			config_setting_set_float(set, config_setting_get_float(src));
		else if(CONFIG_TYPE_STRING == config_setting_type(src))
			config_setting_set_string(set, config_setting_get_string(src));
		else if(CONFIG_TYPE_BOOL == config_setting_type(src))
			config_setting_set_bool(set, config_setting_get_bool(src));
	}
}

void config_setting_copy_elem(config_setting_t * parent, const config_setting_t * src)
{
	config_setting_t * set;
	
	set = NULL;
	if(config_setting_is_aggregate(src))
		config_setting_copy_aggregate(parent, src);
	else if(CONFIG_TYPE_INT == config_setting_type(src))
	{
		set = config_setting_set_int_elem(parent, -1, config_setting_get_int(src));
		config_setting_set_format(set, src->format);
	}
	else if(CONFIG_TYPE_INT64 == config_setting_type(src))
	{
		set = config_setting_set_int64_elem(parent, -1, config_setting_get_int64(src));
		config_setting_set_format(set, src->format);   
	}
	else if(CONFIG_TYPE_FLOAT == config_setting_type(src))
		set = config_setting_set_float_elem(parent, -1, config_setting_get_float(src));
	else if(CONFIG_TYPE_STRING == config_setting_type(src))
		set = config_setting_set_string_elem(parent, -1, config_setting_get_string(src));
	else if(CONFIG_TYPE_BOOL == config_setting_type(src))
		set = config_setting_set_bool_elem(parent, -1, config_setting_get_bool(src));
}

void config_setting_copy_aggregate(config_setting_t * parent, const config_setting_t * src)
{
	config_setting_t * newAgg;
	int i, n;

	newAgg = config_setting_add(parent, config_setting_name(src), config_setting_type(src));
	
	n = config_setting_length(src);	
	for(i = 0; i < n; i++)
	{
		if(config_setting_is_group(src))
		{
			config_setting_copy_simple(newAgg, config_setting_get_elem(src, i));			
		}
		else
		{
			config_setting_copy_elem(newAgg, config_setting_get_elem(src, i));
		}		
	}
}

int config_setting_copy(config_setting_t * parent, const config_setting_t * src)
{
	if((!config_setting_is_group(parent)) &&
	   (!config_setting_is_list(parent)))
		return CONFIG_FALSE;

	if(config_setting_is_aggregate(src))
		config_setting_copy_aggregate(parent, src);
	else
		config_setting_copy_simple(parent, src);
	
	return CONFIG_TRUE;
}
/*
	END
*/

namespace axon
{
	config::config()
	{
		_open = false;
		config_init(&_cfg);
		// config_set_options(&_cfg, CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS | CONFIG_OPTION_COLON_ASSIGNMENT_FOR_NON_GROUPS | CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE);
	}

	config::~config()
	{
		config_destroy(&_cfg);
	}

	config::config(const config& source)
	{
		deepcopy(source);
	}

	config& config::operator=(const config& source)
	{
		if (this == &source)
			return *this;

		deepcopy(source);
		return *this;
	}

	void config::deepcopy(const config& source)
	{
		_open = false;
		config_init(&_cfg);
		
		if (source._open)
		{
			if (access(source._filename.c_str(), F_OK) == -1)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[Copy] Cannot find configuration file (" + source._filename + ")");

			if(!config_read_file(&_cfg, source._filename.c_str()))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[Copy] Error reading config file at line #" + std::to_string(config_error_line(&_cfg)) + " - " + config_error_text(&_cfg));

			_filename = source._filename;
			_path = source._path;
			_master = _root = config_root_setting(&_cfg);
			_open = true;

			std::queue<std::string> crumb = source._crumb;
			while(!crumb.empty())
			{
				open(crumb.front());
				crumb.pop();
			}
		}
	}

	const config_setting_t *config::raw()
	{
		return _root;
	}

	bool config::load(std::string filename)
	{
		_filename = filename;
		
		return load();
	}

	bool config::load()
	{
		DBGPRN("axon::config::load(): %s %s", _filename.c_str(), boost::filesystem::current_path().generic_string().c_str());

		if (_filename.size() <= 1)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Invalid configuration filename");

		if (_filename[0] != '/')
			_filename = boost::filesystem::current_path().generic_string() + "/" + _filename;

		if (access(_filename.c_str(), F_OK) == -1)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot find configuration file (" + _filename + ")");

		if(!config_read_file(&_cfg, _filename.c_str()))
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config file at line #" + std::to_string(config_error_line(&_cfg)) + " - " + config_error_text(&_cfg));

		_master = _root = config_root_setting(&_cfg);

		_open = true;

		return true;
	}

	bool config::reload()
	{
		_path = std::stack<std::string>();
		_crumb = std::queue<std::string>();
		config_destroy(&_cfg);
		config_init(&_cfg);
		load();

		return true;
	}

	bool config::rewind()
	{
		_path = std::stack<std::string>();
		_crumb = std::queue<std::string>();
		// _root = config_root_setting(&_cfg);
		_root = _master;

		return true;
	}

	bool config::save()
	{
		save(_filename);

		return true;
	}

	bool config::save(std::string filename)
	{
		if (filename.size() <= 1)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Invalid configuration filename");

		if(!config_write_file (&_cfg, filename.c_str()))
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error writing configuration to " + filename + " - " + config_error_text(&_cfg));

		return true;
	}

	std::string config::_name(const config_setting_t *setting)
	{
		char *cname;
		std::string sname;

		if ((cname = config_setting_name(setting)) == NULL)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter name");

		sname = cname;

		return sname;
	}

	std::string config::name()
	{
		return _name(_root);
	}

	std::string config::name(int index)
	{
		config_setting_t *setting;

		if ((setting = config_setting_get_elem(_root, index)) == NULL)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter name for index '" + std::to_string(index) + "'");

		return _name(setting);
	}

	bool config::exists(std::string path)
	{
		if (config_lookup(&_cfg, path.c_str()) == NULL)
			return false;
		
		return true;
	}

	bool config::open(std::string path)
	{
		config_setting_t *oldroot = _root;
		if ((_root = config_setting_get_member(oldroot, path.c_str())) == NULL)
		{
			_root = oldroot;
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error opening root config parameter '" + path + "' - ");
		}

		_crumb.push(path);
		_path.push(path);

		return true;
	}

	bool config::close()
	{
		config_setting_t *oldroot = _root;

		if ((_root = config_setting_parent(oldroot)) == NULL)
		{
			_root = oldroot;
			return false;
		}

		_path.pop();

		return true;
	}

	int config::size()
	{
		return config_setting_length(_root);
	}

	bool config::add(const config_setting_t *src)
	{
		if(CONFIG_FALSE == config_setting_copy(_root, src))
		{
			printf("Failed to copy src to dst\n");
		}

		return true;
	}

	bool config::add(const std::string& name, axon::conftype ct)
	{
		if (config_setting_add(_root, name.c_str(), ct) == NULL)
			return false;

		return true;
	}

	bool config::remove(std::string &)
	{
		return true;
	}

	axon::conftype config::type(std::string path)
	{
		config_setting_t *setting;

		if ((setting = config_setting_get_member(_root, path.c_str())) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_INT)
			{
				return INTEGER;
			}
			else if (settingstype == CONFIG_TYPE_INT64)
			{
				return INT64;
			}
			else if (settingstype == CONFIG_TYPE_FLOAT)
			{
				return FLOAT;
			}
			else if (settingstype == CONFIG_TYPE_STRING)
			{
				return STRING;
			}
			else if (settingstype == CONFIG_TYPE_BOOL)
			{
				return BOOL;
			}
			else if (settingstype == CONFIG_TYPE_ARRAY)
			{
				return ARRAY;
			}
			else if (settingstype == CONFIG_TYPE_LIST)
			{
				return LIST;
			}
			else if (settingstype == CONFIG_TYPE_GROUP)
			{
				return GROUP;
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter '" + path + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter '" + path + "'");

		return UNKNOWN;
	}

	axon::conftype config::type(int index)
	{
		config_setting_t *setting;

		if ((setting = config_setting_get_elem(_root, index)) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_INT)
			{
				return INTEGER;
			}
			else if (settingstype == CONFIG_TYPE_STRING)
			{
				return STRING;
			}
			else if (settingstype == CONFIG_TYPE_GROUP)
			{
				return GROUP;
			}
			else if (settingstype == CONFIG_TYPE_ARRAY)
			{
				return ARRAY;
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter index '" + std::to_string(index) + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter index '" + std::to_string(index) + "'");

		return UNKNOWN;
	}

	config::proxy config::get(int index)
	{
		int intval = 0;
		std::string strval;

		config_setting_t *setting;

		if ((setting = config_setting_get_elem(_root, index)) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_INT)
			{
				intval = config_setting_get_int(setting);
			}
			else if (settingstype == CONFIG_TYPE_STRING)
			{
				const char *retval;

				if ((retval = config_setting_get_string(setting)) == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Unexpected error while reading parameter index '" + std::to_string(index) + "'");

				strval = retval;
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter index '" + std::to_string(index) + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter index '" + std::to_string(index) + "'");

		proxy temp(intval, strval);

		return temp;
	}

	config::proxy config::get(std::string path)
	{
		int intval = 0;
		std::string strval;

		config_setting_t *setting;

		if ((setting = config_setting_get_member(_root, path.c_str())) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_INT)
			{
				intval = config_setting_get_int(setting);
			}
			else if (settingstype == CONFIG_TYPE_STRING)
			{
				const char *retval;

				if ((retval = config_setting_get_string(setting)) == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Unexpected error while reading parameter '" + path + "'");

				strval = retval;
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter '" + path + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter '" + path + "'");

		proxy temp(intval, strval);

		return temp;
	}

	bool config::get(std::string path, int& retval)
	{
		config_setting_t *setting;

		if ((setting = config_setting_get_member(_root, path.c_str())) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_INT)
			{
				retval = config_setting_get_int(setting);
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter '" + path + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter '" + path + "'");

		return true;
	}

	bool config::get(std::string path, long long& retval)
	{
		config_setting_t *setting;

		if ((setting = config_setting_get_member(_root, path.c_str())) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_INT64)
			{
				retval = config_setting_get_int64(setting);
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter '" + path + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter '" + path + "'");

		return true;
	}

	bool config::get(std::string path, double& retval)
	{
		config_setting_t *setting;

		if ((setting = config_setting_get_member(_root, path.c_str())) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_FLOAT)
			{
				retval = config_setting_get_float(setting);
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter '" + path + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter '" + path + "'");

		return true;
	}

	bool config::get(std::string path, std::string& retval)
	{
		std::string strval;

		config_setting_t *setting;

		if ((setting = config_setting_get_member(_root, path.c_str())) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_STRING)
			{
				const char *strval;
				if ((strval = config_setting_get_string(setting)) == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Unexpected error while reading parameter '" + path + "'");
				retval = strval;
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter '" + path + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter '" + path + "'");		

		return true;
	}

	bool config::get(std::string path, int index, int& retval)
	{
		config_setting_t *setting;

		if ((setting = config_setting_get_member(_root, path.c_str())) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_ARRAY)
			{
				if ((retval = config_setting_get_int_elem(setting, index)) == 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Unexpected error while reading parameter '" + path + "'");
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter '" + path + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter '" + path + "'");

		return true;
	}

	bool config::get(std::string path, int index, std::string& retval)
	{
		std::string strval;

		config_setting_t *setting;

		if ((setting = config_setting_get_member(_root, path.c_str())) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_ARRAY)
			{
				const char *strval;
				if ((strval = config_setting_get_string_elem(setting, index)) == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Unexpected error while reading parameter '" + path + "'");
				retval = strval;
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter '" + path + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter '" + path + "'");

		return true;
	}

	bool config::get(int index, std::string& retval)
	{
		std::string strval;

		config_setting_t *setting;

		if ((setting = config_setting_get_elem(_root, index)) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_STRING)
			{
				const char *strval;
				if ((strval = config_setting_get_string(setting)) == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Unexpected error while reading parameter index '" + std::to_string(index) + "'");
				retval = strval;
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter index '" + std::to_string(index) + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter index '" + std::to_string(index) + "'");		

		return true;
	}

	bool config::get(int index, int& retval)
	{
		config_setting_t *setting;

		if ((setting = config_setting_get_elem(_root, index)) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_INT)
			{
				retval = config_setting_get_int(setting);
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter index '" + std::to_string(index) + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter index '" + std::to_string(index) + "'");

		return true;
	}

	void config::print(const config_setting_t *settings = NULL , int level = 0)
	{
		if (settings != NULL)
		{

		}
		else if (settings == NULL && _master != NULL)
			settings = _master;
		else
			return;
		
		int count = config_setting_length(settings);
		for (int index = 0; index < count; index++)
		{
			config_setting_t *setting;

			if ((setting = config_setting_get_elem(settings, index)) != NULL)
			{
				int stype = config_setting_type(setting);
				char name[512] = { 0 }, *cname;

				int dval, bval;
				long long llval;
				double fval;

				if ((cname = config_setting_name(setting)) != NULL)
					snprintf(name, 512, "%s", cname);

				switch (stype)
				{
					case CONFIG_TYPE_INT:
						dval = config_setting_get_int(setting);
						std::cout<<std::string(level, ' ')<<index<<") "<<name<<dval<<std::endl;
						break;
					case CONFIG_TYPE_INT64:
						llval = config_setting_get_int64(setting);
						std::cout<<std::string(level, ' ')<<index<<") "<<name<<llval<<std::endl;
						break;
					case CONFIG_TYPE_FLOAT:
						fval = config_setting_get_float(setting);
						std::cout<<std::string(level, ' ')<<index<<") "<<name<<fval<<std::endl;
						break;
					case CONFIG_TYPE_STRING:
						const char *sval;
						if ((sval = config_setting_get_string(setting)) == NULL)
							continue;
						std::cout<<std::string(level, ' ')<<index<<") "<<name<<sval<<std::endl;
						break;
					case CONFIG_TYPE_BOOL:
						bval = config_setting_get_bool(setting);
						std::cout<<std::string(level, ' ')<<index<<") "<<name<<bval<<std::endl;
						break;
					case CONFIG_TYPE_ARRAY:
						std::cout<<std::string(level, ' ')<<index<<") [array] "<<name<<std::endl;
						print(setting, level+1);
						break;
					case CONFIG_TYPE_LIST:
						std::cout<<std::string(level, ' ')<<index<<") [list] "<<name<<std::endl;
						print(setting, level+1);
						break;
					case CONFIG_TYPE_GROUP:
						std::cout<<std::string(level, ' ')<<index<<") [group] "<<name<<std::endl;
						print(setting, level+1);
						break;
					default:
						std::cout<<index<<") "<<cname<<" is unknown type"<<std::endl;
						break;
				}
			}
			else
				std::cout<<index<<") unknown"<<std::endl;
		}
	}

	void config::print()
	{
		print(NULL, 0);
	}
}