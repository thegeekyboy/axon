#include <axon.h>
#include <axon/config.h>

namespace axon
{
	config::config()
	{
		_open = false;
		config_init(&_cfg);
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
			_root = config_root_setting(&_cfg);
			_open = true;

			std::vector<std::string> tq;
			std::stack<std::string> ts = source._path;

			while (!ts.empty())
			{
				std::string tstr = ts.top();
				tq.push_back(tstr);
				ts.pop();
			}

			for (int i = tq.size()-1; i >= 0; --i)
				open(tq[i]);
		}
	}

	bool config::load(std::string filename)
	{
		_filename = filename;
		
		return load();
	}

	bool config::load()
	{
		if (_filename.size() <= 1)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Invalid configuration filename");

		if (access(_filename.c_str(), F_OK) == -1)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot find configuration file (" + _filename + ")");

		if(!config_read_file(&_cfg, _filename.c_str()))
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config file at line #" + std::to_string(config_error_line(&_cfg)) + " - " + config_error_text(&_cfg));

		_root = config_root_setting(&_cfg);

		_open = true;

		return true;
	}

	bool config::open(std::string path)
	{
		config_setting_t *oldroot = _root;

		if ((_root = config_setting_get_member(oldroot, path.c_str())) == NULL)
		{
			_root = oldroot;
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error opening root config parameter '" + path + "'");
		}

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

	bool config::reload()
	{
		config_destroy(&_cfg);
		config_init(&_cfg);
		load();

		return true;
	}

	std::string config::name(int index)
	{
		char *cname;
		std::string sname;

		if ((_setting = config_setting_get_elem(_root, index)) == NULL)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter name for index '" + std::to_string(index) + "'");

		if ((cname = config_setting_name(_setting)) == NULL)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter name");

		sname = cname;

		return sname;
	}

	int config::size()
	{
		return config_setting_length(_root);
	}

	axon::conftype config::type(std::string path)
	{
		config_setting_t *setting;

		if ((setting = config_setting_get_member(_root, path.c_str())) != NULL)
		{
			int settingstype = config_setting_type(setting);

			if (settingstype == CONFIG_TYPE_INT)
			{
				_setting = setting;
				return INTEGER;
			}
			else if (settingstype == CONFIG_TYPE_STRING)
			{
				_setting = setting;
				return STRING;
			}
			else if (settingstype == CONFIG_TYPE_GROUP)
			{
				_setting = setting;				
				return GROUP;
			}
			else if (settingstype == CONFIG_TYPE_ARRAY)
			{
				_setting = setting;
				return ARRAY;
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
				_setting = setting;

				return INTEGER;
			}
			else if (settingstype == CONFIG_TYPE_STRING)
			{
				_setting = setting;				
				return STRING;
			}
			else if (settingstype == CONFIG_TYPE_GROUP)
			{
				_setting = setting;				
				return GROUP;
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter index '" + std::to_string(index) + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter index '" + std::to_string(index) + "'");

		return UNKNOWN;
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

		_setting = setting;

		return temp;
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

		_setting = setting;

		return true;
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

		_setting = setting;

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

		_setting = setting;

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
				const char *strval;
				if ((retval = config_setting_get_int_elem(setting, index)) == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Unexpected error while reading parameter '" + path + "'");
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Config parameter '" + path + "' is of unsupported type (" + std::to_string(settingstype) + ")");
		}
		else
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error reading config parameter '" + path + "'");

		_setting = setting;

		return true;
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

		_setting = setting;

		return temp;
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

		_setting = setting;

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

		_setting = setting;

		return true;
	}
}