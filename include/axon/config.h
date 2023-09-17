#ifndef AXON_CONFIG_H_
#define AXON_CONFIG_H_

#include <stack>
#include <queue>

#include <string.h>
#include <unistd.h>

#include <libconfig.h>

namespace axon
{

	enum conftype
	{
		INTEGER = 0,
		STRING,
		GROUP,
		ARRAY,
		UNKNOWN,
		MISSING
	};

	class config
	{
		std::string _filename, _error;

		config_t _cfg;
		config_setting_t *_setting;
		config_setting_t *_root;

		bool _open;

		std::stack<std::string> _path;
		std::queue<std::string> _crumb;

		void deepcopy(const config &);

	public:
		class proxy
		{
			int intval;
			char strval[1024];

		public:
			proxy(int intv, std::string strv)
			{
				intval = intv;
				strcpy(strval, strv.c_str());
			};
			operator int() const { return intval; }
			operator char *() const { return const_cast<char *>(strval); }
		};

		config();
		~config();

		config(const config &);
		config &operator=(const config &);

		bool load(std::string);
		bool load();
		bool reload();

		bool open(std::string path);
		bool close();
		std::string name(int);
		int size();

		axon::conftype type(std::string);
		axon::conftype type(int);

		proxy get(std::string);
		bool get(std::string, std::string &);
		bool get(std::string, int &);

		bool get(std::string, int, std::string &);
		bool get(std::string, int, int &);

		proxy get(int);
		bool get(int, std::string &);
		bool get(int, int &);
	};

} // namespace axon
#endif